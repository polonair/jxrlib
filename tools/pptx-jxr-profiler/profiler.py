#!/usr/bin/env python3
"""Read-only PPTX/JPEG XR inventory and codestream profile reporter."""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
import os
import posixpath
import sqlite3
import struct
import sys
import time
import uuid
import zipfile
from collections import defaultdict
from datetime import datetime, timezone
from pathlib import Path
from typing import Any
from xml.etree import ElementTree

VERSION = "0.1.4"
CACHE_SCHEMA = 1
JXR_SIGNATURE = b"WMPHOTO\x00"
JXR_EXTENSIONS = {".jxr", ".wdp", ".hdp", ".hdr", ".jpegxr"}
MAX_XML_BYTES = 8 * 1024 * 1024
MAX_PROFILE_BYTES_DEFAULT = 512 * 1024 * 1024

DEPTH_NAMES = {
    0: "1bpp", 1: "8bpp", 2: "16bpp", 3: "16bpp-fixed",
    4: "16bpp-float", 5: "32bpp", 6: "32bpp-fixed",
    7: "32bpp-float", 8: "5bpp", 9: "10bpp", 10: "16bpp-565",
    15: "1bpp-alternate-polarity",
}
COLOR_NAMES = {
    0: "Y_ONLY", 1: "YUV_420", 2: "YUV_422", 3: "YUV_444",
    4: "CMYK", 5: "reserved", 6: "NCOMPONENT", 7: "CF_RGB",
    8: "CF_RGBE",
}
SUBBAND_NAMES = {
    0: "all", 1: "no_flexbits", 2: "no_highpass", 3: "dc_only",
    4: "isolated/unsupported",
}
ORIENTATION_NAMES = {
    0: "none", 1: "flip_vertical", 2: "flip_horizontal",
    3: "flip_both", 4: "rotate_90_cw", 5: "rotate_90_cw_flip_vertical",
    6: "rotate_90_cw_flip_horizontal", 7: "rotate_90_cw_flip_both",
}
GUID_NAMES = {
    "6fddc324-4e03-4bfe-b185-3d77768dc905": "BlackWhite1",
    "6fddc324-4e03-4bfe-b185-3d77768dc908": "Gray8",
    "6fddc324-4e03-4bfe-b185-3d77768dc909": "Rgb555",
    "6fddc324-4e03-4bfe-b185-3d77768dc90a": "Rgb565",
    "6fddc324-4e03-4bfe-b185-3d77768dc90b": "Gray16",
    "6fddc324-4e03-4bfe-b185-3d77768dc90c": "Bgr24",
    "6fddc324-4e03-4bfe-b185-3d77768dc90d": "Rgb24",
    "6fddc324-4e03-4bfe-b185-3d77768dc90e": "Bgr32",
    "6fddc324-4e03-4bfe-b185-3d77768dc90f": "Bgra32",
    "f5c7ad2d-6a8d-43dd-a7a8-a29935261ae9": "Rgba32",
    "6fddc324-4e03-4bfe-b185-3d77768dc915": "Rgb48",
    "6fddc324-4e03-4bfe-b185-3d77768dc916": "Rgba64",
}


class ProfileError(Exception):
    pass


class BitReader:
    def __init__(self, data: bytes, bit: int = 0):
        self.data = data
        self.bit = bit

    def read(self, count: int) -> int:
        if count < 0 or count > 32 or self.bit + count > len(self.data) * 8:
            raise ProfileError("truncated codestream header")
        value = 0
        for _ in range(count):
            value = (value << 1) | ((self.data[self.bit >> 3] >>
                                    (7 - (self.bit & 7))) & 1)
            self.bit += 1
        return value

    def align_byte(self) -> None:
        self.bit = (self.bit + 7) & ~7


def _read_quantizer(reader: BitReader, channels: int) -> dict[str, Any]:
    mode = reader.read(2) if channels > 1 else 0
    first = reader.read(8)
    values = [first]
    if mode == 1:
        values.append(reader.read(8))
    elif mode > 1:
        values.extend(reader.read(8) for _ in range(channels - 1))
    if len(values) == 1 and channels > 1:
        effective = values * channels
    elif mode == 1 and channels > 2:
        effective = [values[0], values[1]] + [values[1]] * (channels - 2)
    else:
        effective = values[:]
    return {"channel_mode": mode, "stored_indices": values,
            "effective_indices": effective}


def _read_quantizer_header(reader: BitReader, channels: int,
                           subband: int) -> dict[str, Any]:
    result: dict[str, Any] = {}
    if reader.read(1):
        result["dc"] = _read_quantizer(reader, channels)
    else:
        result["dc"] = {"inherits": "codec_default"}
    if subband != 3:
        if reader.read(1) == 0:
            if reader.read(1):
                result["lp"] = _read_quantizer(reader, channels)
            else:
                result["lp"] = {"inherits": "dc"}
        else:
            result["lp"] = {"inherits": "dc"}
        if subband != 2:
            if reader.read(1) == 0:
                if reader.read(1):
                    result["hp"] = _read_quantizer(reader, channels)
                else:
                    result["hp"] = {"inherits": "lp"}
            else:
                result["hp"] = {"inherits": "lp"}
    return result


def parse_codestream(data: bytes) -> dict[str, Any]:
    """Parse the main, image-plane and frame-quantizer headers only."""
    if not data.startswith(JXR_SIGNATURE):
        raise ProfileError("missing WMPHOTO codestream signature")
    reader = BitReader(data, len(JXR_SIGNATURE) * 8)
    version = reader.read(4)
    subversion = reader.read(4)
    if version != 1:
        raise ProfileError("unsupported codestream version %d" % version)
    tiled = bool(reader.read(1))
    bitstream_format = reader.read(1)
    orientation = reader.read(3)
    index_table = bool(reader.read(1))
    overlap = reader.read(2)
    if overlap == 3:
        raise ProfileError("reserved overlap value")
    abbreviated = bool(reader.read(1))
    coded_bit_depth = reader.read(1)
    windowed = bool(reader.read(1))
    trim_flexbits = bool(reader.read(1))
    tile_stretch = bool(reader.read(1))
    red_blue_swapped = bool(reader.read(1))
    reserved = reader.read(1)
    has_alpha = bool(reader.read(1))
    source_color_format = reader.read(4)
    source_bit_depth = reader.read(4)
    width = reader.read(16 if abbreviated else 32) + 1
    height = reader.read(16 if abbreviated else 32) + 1
    vertical_tiles = horizontal_tiles = 1
    tile_columns: list[int] = []
    tile_rows: list[int] = []
    if tiled:
        vertical_tiles = reader.read(12) + 1
        horizontal_tiles = reader.read(12) + 1
    columns_mb = (width + 15) // 16
    rows_mb = (height + 15) // 16
    tile_dimension_bits = 8 if abbreviated else 16
    if tiled:
        x = 0
        for _ in range(vertical_tiles - 1):
            x += reader.read(tile_dimension_bits)
            tile_columns.append(x)
        y = 0
        for _ in range(horizontal_tiles - 1):
            y += reader.read(tile_dimension_bits)
            tile_rows.append(y)
    if tile_stretch:
        reader.read(8 * vertical_tiles * horizontal_tiles)
    extras = {"top": 0, "left": 0,
              "bottom": (16 - height % 16) % 16,
              "right": (16 - width % 16) % 16}
    if windowed:
        extras = {key: reader.read(6) for key in
                  ("top", "left", "bottom", "right")}
    reader.align_byte()

    color_format = reader.read(3)
    scaled_arithmetic = bool(reader.read(1))
    subband = reader.read(4)
    channels = {0: 1, 1: 3, 2: 3, 3: 3, 4: 4, 6: reader.read(4) + 1}.get(color_format)
    if channels is None:
        raise ProfileError("unsupported color format %d" % color_format)
    chroma_centering: dict[str, int] = {}
    if color_format == 1:
        reader.read(1)
        chroma_centering["x"] = reader.read(3)
        reader.read(1)
        chroma_centering["y"] = reader.read(3)
    elif color_format == 2:
        reader.read(1)
        chroma_centering["x"] = reader.read(3)
        reader.read(4)
    elif color_format in (3, 6):
        reader.read(4)
        reader.read(4)
    elif color_format == 4:
        pass
    sample_conversion = source_bit_depth in (2, 3, 5, 6, 7)
    shift_or_mantissa = reader.read(8) if sample_conversion else None
    exponent_bias = reader.read(8) if source_bit_depth == 7 else None
    quantizers = _read_quantizer_header(reader, channels, subband)
    reader.align_byte()
    header_bytes = reader.bit // 8
    profile: dict[str, Any] = {
        "codestream_version": version,
        "codestream_subversion": subversion,
        "coded_bit_depth_code": coded_bit_depth,
        "coded_bit_depth": {0: "short", 1: "long"}.get(coded_bit_depth, "unknown"),
        "bitstream_layout": {0: "spatial", 1: "frequency"}.get(
            bitstream_format, "unknown"),
        "width": width,
        "height": height,
        "source_color_format_code": source_color_format,
        "source_color_format": COLOR_NAMES.get(source_color_format, "unknown"),
        "source_bit_depth_code": source_bit_depth,
        "source_bit_depth": DEPTH_NAMES.get(source_bit_depth, "reserved/unknown"),
        "orientation_code": orientation,
        "orientation": ORIENTATION_NAMES.get(orientation, "unknown"),
        "overlap": overlap,
        "has_alpha": has_alpha,
        "alpha_mode": "present_in_codestream" if has_alpha else "none",
        "trim_flexbits_flag": trim_flexbits,
        "red_blue_swapped": red_blue_swapped,
        "index_table": index_table,
        "tile_mode": "hard" if subversion == 9 else ("soft" if tiled else "none"),
        "tile_columns": vertical_tiles,
        "tile_rows": horizontal_tiles,
        "tile_column_boundaries_mb": tile_columns,
        "tile_row_boundaries_mb": tile_rows,
        "image_macroblock_columns": columns_mb,
        "image_macroblock_rows": rows_mb,
        "window_extras": extras,
        "plane_color_format_code": color_format,
        "plane_color_format": COLOR_NAMES.get(color_format, "unknown"),
        "channel_count": channels,
        "scaled_arithmetic": scaled_arithmetic,
        "subband_code": subband,
        "subbands": SUBBAND_NAMES.get(subband, "unknown"),
        "chroma_centering": chroma_centering,
        "sample_conversion": sample_conversion,
        "shift_or_mantissa": shift_or_mantissa,
        "exponent_bias": exponent_bias,
        "frame_quantizers": quantizers,
        "profile_quantizer_scope": "frame_header_only",
        "tile_quantizer_scope": "not_parsed",
        "tile_quantizer_note": "Tile packet headers may override frame quantizers.",
        "frame_header_bytes": header_bytes,
        "frame_header_parse_complete": True,
        "reserved_header_bit": reserved,
    }
    if source_bit_depth in (0, 15):
        profile["black_white_depth_variant"] = "alternate" if source_bit_depth == 15 else "base"
    return profile


def _tiff_value(data: bytes, endian: str, type_id: int, count: int,
                value_field: int) -> bytes | None:
    sizes = {1: 1, 2: 1, 3: 2, 4: 4, 5: 8, 6: 1, 7: 1,
             8: 2, 9: 4, 10: 8, 11: 4, 12: 8}
    size = sizes.get(type_id)
    if size is None or count < 0 or count > 1_000_000:
        return None
    length = size * count
    if length <= 4:
        offset = value_field
    else:
        offset = struct.unpack(endian + "I", data[value_field:value_field + 4])[0]
    if offset < 0 or offset + length > len(data):
        return None
    return data[offset:offset + length]


def parse_jxr(payload: bytes) -> dict[str, Any]:
    wrapper: dict[str, Any] = {"container": "raw_codestream", "pixel_format_guid": None,
                               "pixel_format": None, "container_width": None,
                               "container_height": None, "orientation_tag": None,
                               "codestream_offset": 0, "codestream_length": len(payload),
                               "alpha_offset": None, "alpha_byte_count": None,
                               "alpha_end_offset": None,
                               "alpha_range_interpretation": None}
    offset = 0
    length = len(payload)
    alpha_offset = None
    if payload.startswith(b"II\xbc\x01") or payload.startswith(b"MM\x01\xbc"):
        little = payload[:2] == b"II"
        endian = "<" if little else ">"
        if len(payload) < 10:
            raise ProfileError("truncated TIFF-like JPEG XR container header")
        ifd_offset = struct.unpack(endian + "I", payload[4:8])[0]
        if ifd_offset + 2 > len(payload):
            raise ProfileError("container IFD offset is outside the entry")
        count = struct.unpack(endian + "H", payload[ifd_offset:ifd_offset + 2])[0]
        if count > 4096 or ifd_offset + 2 + count * 12 > len(payload):
            raise ProfileError("invalid container IFD entry count")
        tags: dict[int, tuple[int, int, int]] = {}
        for index in range(count):
            pos = ifd_offset + 2 + index * 12
            tag, typ = struct.unpack(endian + "HH", payload[pos:pos + 4])
            item_count = struct.unpack(endian + "I", payload[pos + 4:pos + 8])[0]
            field = pos + 8
            tags[tag] = (typ, item_count, field)
        def scalar(tag: int) -> int | None:
            item = tags.get(tag)
            if item is None:
                return None
            typ, item_count, field = item
            raw = _tiff_value(payload, endian, typ, item_count, field)
            if raw is None or not raw:
                return None
            if typ in (3, 8) and len(raw) >= 2:
                return struct.unpack(endian + "H", raw[:2])[0]
            if typ in (4, 9) and len(raw) >= 4:
                return struct.unpack(endian + "I", raw[:4])[0]
            return raw[0]
        def floating(tag: int) -> float | None:
            item = tags.get(tag)
            if item is None or item[0] != 11 or item[1] != 1:
                return None
            raw = _tiff_value(payload, endian, item[0], item[1], item[2])
            if raw is None or len(raw) != 4:
                return None
            return struct.unpack(endian + "f", raw)[0]
        item = tags.get(0xBC01)
        guid = None
        if item:
            raw = _tiff_value(payload, endian, item[0], item[1], item[2])
            if raw and len(raw) >= 16:
                try:
                    guid = str(uuid.UUID(bytes_le=raw[:16])).lower()
                except ValueError:
                    pass
        width = scalar(0xBC80)
        height = scalar(0xBC81)
        offset = scalar(0xBCC0)
        length = scalar(0xBCC1)
        alpha_offset = scalar(0xBCC2)
        alpha_range_tag = scalar(0xBCC3)
        if offset is None or length is None or offset + length > len(payload):
            raise ProfileError("container codestream offset/length is invalid")
        if (alpha_offset is None) != (alpha_range_tag is None):
            raise ProfileError("container has incomplete planar alpha metadata")
        alpha_end_offset = None
        alpha_byte_count = None
        alpha_range_interpretation = None
        if alpha_offset is not None and alpha_range_tag is not None:
            if alpha_range_tag > alpha_offset:
                alpha_end_offset = alpha_range_tag
                alpha_range_interpretation = "absolute_end_offset"
            else:
                alpha_end_offset = alpha_offset + alpha_range_tag
                alpha_range_interpretation = "byte_count"
            alpha_byte_count = alpha_end_offset - alpha_offset
            if alpha_byte_count <= 0 or alpha_end_offset > len(payload):
                raise ProfileError("container alpha range is invalid")
        wrapper.update({"container": "tiff_like_jxr", "pixel_format_guid": guid,
                        "pixel_format": GUID_NAMES.get(guid, "unknown"),
                        "container_width": width, "container_height": height,
                        "orientation_tag": scalar(0xBC02),
                        "horizontal_dpi": floating(0xBC82),
                        "vertical_dpi": floating(0xBC83),
                        "codestream_offset": offset, "codestream_length": length,
                        "alpha_offset": alpha_offset,
                        "alpha_end_offset": alpha_end_offset,
                        "alpha_byte_count": alpha_byte_count,
                        "alpha_range_tag_value": alpha_range_tag,
                        "alpha_range_interpretation": alpha_range_interpretation})
    if length < len(JXR_SIGNATURE):
        raise ProfileError("JPEG XR codestream is too short")
    main_data = payload[offset:offset + length]
    profile = parse_codestream(main_data)
    wrapper["frame_header_offset"] = offset
    wrapper.update(profile)
    if alpha_offset is not None:
        alpha_end = wrapper["alpha_end_offset"]
        if alpha_end is None or alpha_end <= alpha_offset or alpha_end > len(payload):
            raise ProfileError("container alpha range is invalid")
        alpha_bytes = payload[alpha_offset:alpha_end]
        alpha_profile = parse_codestream(alpha_bytes)
        wrapper["alpha_mode"] = "planar"
        wrapper["alpha_profile"] = alpha_profile
    elif wrapper["has_alpha"]:
        wrapper["alpha_mode"] = "interleaved_or_embedded"
    return wrapper


def is_jxr_signature(prefix: bytes) -> bool:
    return prefix.startswith(JXR_SIGNATURE) or prefix.startswith(b"II\xbc\x01") or \
        prefix.startswith(b"MM\x01\xbc")


def _part_from_relationships_name(name: str) -> str:
    parent, base = posixpath.split(name)
    if posixpath.basename(parent) != "_rels" or not base.endswith(".rels"):
        raise ValueError("not a relationship part")
    source_name = base[:-5]
    source_parent = posixpath.dirname(parent)
    return posixpath.join(source_parent, source_name) if source_parent else source_name


def _safe_target(source_part: str, target: str) -> str | None:
    if target.startswith("/"):
        resolved = posixpath.normpath(target.lstrip("/"))
    else:
        resolved = posixpath.normpath(posixpath.join(posixpath.dirname(source_part), target))
    if resolved == ".." or resolved.startswith("../"):
        return None
    return resolved


def scan_relationships(archive: zipfile.ZipFile, names: list[str],
                      max_xml_bytes: int = MAX_XML_BYTES) -> tuple[dict[str, list[dict[str, str]]], list[dict[str, str]]]:
    rels: dict[str, dict[str, dict[str, str]]] = {}
    issues: list[dict[str, str]] = []
    for name in names:
        if not name.endswith(".rels") or not ("/_rels/" in name or name.startswith("_rels/")):
            continue
        info = archive.getinfo(name)
        if info.file_size > max_xml_bytes:
            issues.append({"entry": name, "stage": "relationships", "error": "XML size limit"})
            continue
        try:
            source_part = _part_from_relationships_name(name)
            root = ElementTree.fromstring(archive.read(name))
            mapping: dict[str, dict[str, str]] = {}
            for element in root.iter():
                if element.tag.rsplit("}", 1)[-1] != "Relationship":
                    continue
                rid = element.attrib.get("Id")
                target = element.attrib.get("Target")
                mode = element.attrib.get("TargetMode", "Internal")
                if not rid or not target:
                    continue
                resolved = None if mode.casefold() == "external" else _safe_target(source_part, target)
                mapping[rid] = {"target": resolved or target, "external": str(mode.casefold() == "external"),
                                "type": element.attrib.get("Type", "")}
            rels[source_part] = mapping
        except (ElementTree.ParseError, ValueError, KeyError, OSError) as exc:
            issues.append({"entry": name, "stage": "relationships", "error": str(exc)})

    links: dict[str, list[dict[str, str]]] = defaultdict(list)
    for name in names:
        if not name.lower().endswith(".xml") or "/_rels/" in name:
            continue
        info = archive.getinfo(name)
        if info.file_size > max_xml_bytes:
            continue
        try:
            root = ElementTree.fromstring(archive.read(name))
        except (ElementTree.ParseError, OSError, zipfile.BadZipFile) as exc:
            issues.append({"entry": name, "stage": "xml_links", "error": str(exc)})
            continue
        mapping = rels.get(name, {})
        for element in root.iter():
            local = element.tag.rsplit("}", 1)[-1]
            if local not in ("blip", "imgLayer"):
                continue
            role = "original_imgLayer" if local == "imgLayer" else "display_blip"
            for attr, rid in element.attrib.items():
                if attr.rsplit("}", 1)[-1] not in ("embed", "link"):
                    continue
                rel = mapping.get(rid)
                if not rel or rel["external"] == "True":
                    continue
                links[rel["target"]].append({"source_part": name, "relationship_id": rid,
                                              "role": role, "relationship_type": rel["type"]})
    return links, issues


def _read_content_types(archive: zipfile.ZipFile, names: list[str]) -> dict[str, str]:
    result: dict[str, str] = {}
    if "[Content_Types].xml" not in names:
        return result
    info = archive.getinfo("[Content_Types].xml")
    if info.file_size > MAX_XML_BYTES:
        return result
    try:
        root = ElementTree.fromstring(archive.read("[Content_Types].xml"))
        defaults: dict[str, str] = {}
        for element in root.iter():
            local = element.tag.rsplit("}", 1)[-1]
            if local == "Default" and "Extension" in element.attrib:
                defaults[element.attrib["Extension"].casefold()] = element.attrib.get("ContentType", "")
            elif local == "Override" and "PartName" in element.attrib:
                result[element.attrib["PartName"].lstrip("/")] = element.attrib.get("ContentType", "")
        for name in names:
            result.setdefault(name, defaults.get(Path(name).suffix.lstrip(".").casefold(), ""))
    except (ElementTree.ParseError, OSError, zipfile.BadZipFile):
        pass
    return result


def _hash(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def scan_package(path: Path, relative_path: str, max_profile_bytes: int) -> tuple[list[dict[str, Any]], list[dict[str, str]], int]:
    assets: list[dict[str, Any]] = []
    issues: list[dict[str, str]] = []
    entries_seen = 0
    try:
        with zipfile.ZipFile(path, "r") as archive:
            infos = archive.infolist()
            names = [item.filename for item in infos if not item.is_dir()]
            content_types = _read_content_types(archive, names)
            relationships, rel_issues = scan_relationships(archive, names)
            issues.extend({"pptx": relative_path, **item} for item in rel_issues)
            for info in infos:
                if info.is_dir():
                    continue
                entries_seen += 1
                if info.flag_bits & 1:
                    issues.append({"pptx": relative_path, "entry": info.filename,
                                   "stage": "zip", "error": "encrypted ZIP entry"})
                    continue
                try:
                    with archive.open(info, "r") as stream:
                        prefix = stream.read(8)
                    signature_candidate = is_jxr_signature(prefix)
                    extension_candidate = Path(info.filename).suffix.casefold() in JXR_EXTENSIONS
                    if not signature_candidate and not extension_candidate:
                        continue
                    if info.file_size > max_profile_bytes:
                        issues.append({"pptx": relative_path, "entry": info.filename,
                                       "stage": "jxr_read", "error": "entry exceeds --max-entry-mb"})
                        continue
                    with archive.open(info, "r") as stream:
                        payload = stream.read(max_profile_bytes + 1)
                    if len(payload) > max_profile_bytes:
                        raise ProfileError("entry exceeds configured read limit")
                    signature = is_jxr_signature(payload[:8])
                    if not signature:
                        if extension_candidate:
                            issues.append({"pptx": relative_path, "entry": info.filename,
                                           "stage": "signature", "error": "JXR-like extension but no JXR signature"})
                        continue
                    digest = _hash(payload)
                    try:
                        profile = parse_jxr(payload)
                        parse_status = "parsed"
                        parse_error = ""
                    except (ProfileError, struct.error, OverflowError, IndexError, ValueError) as exc:
                        profile = {}
                        parse_status = "parse_error"
                        parse_error = str(exc)
                        issues.append({"pptx": relative_path, "entry": info.filename,
                                       "stage": "jxr_parse", "error": parse_error})
                    assets.append({
                        "pptx": relative_path,
                        "entry": info.filename,
                        "sha256": digest,
                        "compressed_bytes": info.compress_size,
                        "uncompressed_bytes": info.file_size,
                        "extension": Path(info.filename).suffix.casefold(),
                        "content_type": content_types.get(info.filename, ""),
                        "detected_by_signature": signature,
                        "parse_status": parse_status,
                        "parse_error": parse_error,
                        "relationships": relationships.get(info.filename, []),
                        "profile": profile,
                    })
                except (OSError, RuntimeError, zipfile.BadZipFile, ProfileError) as exc:
                    issues.append({"pptx": relative_path, "entry": info.filename,
                                   "stage": "entry_read", "error": str(exc)})
    except (OSError, zipfile.BadZipFile, zipfile.LargeZipFile, RuntimeError) as exc:
        issues.append({"pptx": relative_path, "entry": "", "stage": "pptx_open", "error": str(exc)})
    return assets, issues, entries_seen


def feature_profile(profile: dict[str, Any]) -> str:
    fields = ("source_color_format", "source_bit_depth", "orientation", "overlap",
              "has_alpha", "alpha_mode", "bitstream_layout", "tile_mode",
              "tile_columns", "tile_rows", "subbands", "scaled_arithmetic",
              "sample_conversion", "shift_or_mantissa", "channel_count",
              "frame_quantizers")
    selected = {key: profile.get(key) for key in fields}
    selected["frame_quantizers"] = json.dumps(profile.get("frame_quantizers", {}),
                                               sort_keys=True, separators=(",", ":"))
    return json.dumps(selected, sort_keys=True,
                      separators=(",", ":"))


def _profile_row(profile_key: str, assets: list[dict[str, Any]]) -> dict[str, Any]:
    decoded = json.loads(profile_key)
    return {**decoded, "unique_assets": len({asset["sha256"] for asset in assets}),
            "occurrences": len(assets),
            "presentations": len({asset["pptx"] for asset in assets}),
            "examples": " | ".join(asset["pptx"] + "::" + asset["entry"]
                                    for asset in assets[:3])}


def _write_json(path: Path, value: Any) -> None:
    temporary = path.with_suffix(path.suffix + ".tmp")
    temporary.write_text(json.dumps(value, ensure_ascii=False, indent=2) + "\n",
                         encoding="utf-8")
    temporary.replace(path)


def write_reports(out: Path, root: Path, assets: list[dict[str, Any]],
                  issues: list[dict[str, str]], pptx_count: int,
                  entries_count: int, cached_count: int, started: float,
                  end_time: str) -> None:
    out.mkdir(parents=True, exist_ok=True)
    by_hash: dict[str, list[dict[str, Any]]] = defaultdict(list)
    by_profile: dict[str, list[dict[str, Any]]] = defaultdict(list)
    for asset in assets:
        by_hash[asset["sha256"]].append(asset)
        if asset["parse_status"] == "parsed":
            by_profile[feature_profile(asset["profile"])].append(asset)
    unique_assets = []
    for digest, occurrences in sorted(by_hash.items()):
        representative = dict(occurrences[0])
        representative["occurrences"] = len(occurrences)
        representative["presentations"] = sorted({item["pptx"] for item in occurrences})
        unique_assets.append(representative)
    profile_rows = [_profile_row(key, group) for key, group in by_profile.items()]
    profile_rows.sort(key=lambda row: (-row["occurrences"], row["source_bit_depth"] or ""))
    summary = {
        "scanner_version": VERSION,
        "root": str(root),
        "started_utc": datetime.fromtimestamp(started, timezone.utc).isoformat(),
        "finished_utc": end_time,
        "elapsed_seconds": round(time.time() - started, 3),
        "pptx_files": pptx_count,
        "zip_entries_seen": entries_count,
        "jxr_occurrences": len(assets),
        "unique_jxr_assets": len(by_hash),
        "parsed_occurrences": sum(a["parse_status"] == "parsed" for a in assets),
        "parse_failures": sum(a["parse_status"] != "parsed" for a in assets),
        "issues": len(issues),
        "cached_pptx": cached_count,
        "feature_profiles": len(profile_rows),
    }
    _write_json(out / "run.json", summary)
    assets_jsonl = out / "assets.jsonl"
    assets_temp = assets_jsonl.with_suffix(".jsonl.tmp")
    with assets_temp.open("w", encoding="utf-8") as handle:
        for item in unique_assets:
            handle.write(json.dumps(item, ensure_ascii=False, sort_keys=True) + "\n")
    assets_temp.replace(assets_jsonl)
    _write_json(out / "occurrences.json", assets)
    _write_json(out / "profiles.json", profile_rows)
    _write_json(out / "errors.json", issues)

    occurrence_columns = ["pptx", "entry", "sha256", "parse_status", "parse_error",
                          "compressed_bytes", "uncompressed_bytes", "extension",
                          "content_type", "detected_by_signature", "relationships", "profile"]
    with (out / "occurrences.csv").open("w", newline="", encoding="utf-8-sig") as handle:
        writer = csv.DictWriter(handle, fieldnames=occurrence_columns, extrasaction="ignore")
        writer.writeheader()
        for asset in assets:
            row = dict(asset)
            row["relationships"] = json.dumps(row.get("relationships", []), ensure_ascii=False)
            row["profile"] = json.dumps(row.get("profile", {}), ensure_ascii=False,
                                        sort_keys=True)
            writer.writerow(row)
    if profile_rows:
        profile_columns = list(profile_rows[0].keys())
        with (out / "profiles.csv").open("w", newline="", encoding="utf-8-sig") as handle:
            writer = csv.DictWriter(handle, fieldnames=profile_columns, extrasaction="ignore")
            writer.writeheader()
            writer.writerows(profile_rows)
    else:
        (out / "profiles.csv").write_text("\n", encoding="utf-8-sig")
    with (out / "errors.csv").open("w", newline="", encoding="utf-8-sig") as handle:
        columns = ["pptx", "entry", "stage", "error"]
        writer = csv.DictWriter(handle, fieldnames=columns, extrasaction="ignore")
        writer.writeheader()
        writer.writerows(issues)

    lines = ["# PPTX JPEG XR scan", "", "## Summary", ""]
    lines.extend("- **%s:** %s" % (key, value) for key, value in summary.items())
    lines.extend(["", "## Most common parsed feature profiles", "",
                  "| Occurrences | Unique JXR | PPTX files | Bit depth | Color | Layout | Overlap | Alpha | Frame QP syntax | Tiles | Example |",
                  "|---:|---:|---:|---|---|---|---:|---|---|---|---|"])
    for row in profile_rows[:50]:
        example = row["examples"].replace("|", "\\|")
        lines.append("| %d | %d | %d | %s | %s | %s | %s | %s | `%s` | %sx%s %s | `%s` |" % (
            row["occurrences"], row["unique_assets"], row["presentations"],
            row.get("source_bit_depth"), row.get("source_color_format"),
            row.get("bitstream_layout"), row.get("overlap"), row.get("alpha_mode"),
            row.get("frame_quantizers"),
            row.get("tile_columns"), row.get("tile_rows"), row.get("tile_mode"), example))
    lines.extend(["", "Detailed data: `occurrences.csv`, `assets.jsonl`, `profiles.csv`, `errors.csv`.",
                  "The scanner reads packages without extracting or modifying them.", ""])
    (out / "summary.md").write_text("\n".join(lines), encoding="utf-8")


def _cache_open(path: Path) -> sqlite3.Connection:
    path.parent.mkdir(parents=True, exist_ok=True)
    connection = sqlite3.connect(str(path))
    connection.execute("CREATE TABLE IF NOT EXISTS packages ("
                       "path TEXT PRIMARY KEY, size INTEGER NOT NULL, mtime_ns INTEGER NOT NULL,"
                       "version TEXT NOT NULL, assets TEXT NOT NULL, issues TEXT NOT NULL,"
                       "entries INTEGER NOT NULL)")
    connection.commit()
    return connection


def _cached(connection: sqlite3.Connection, relative: str, path: Path,
            max_profile_bytes: int) -> tuple[list[dict[str, Any]], list[dict[str, str]], int] | None:
    stat = path.stat()
    row = connection.execute("SELECT size,mtime_ns,version,assets,issues,entries FROM packages WHERE path=?",
                             (relative,)).fetchone()
    cache_version = "%s:%d" % (VERSION, max_profile_bytes)
    if not row or row[0] != stat.st_size or row[1] != stat.st_mtime_ns or row[2] != cache_version:
        return None
    return json.loads(row[3]), json.loads(row[4]), row[5]


def _cache_put(connection: sqlite3.Connection, relative: str, path: Path,
                result: tuple[list[dict[str, Any]], list[dict[str, str]], int],
                max_profile_bytes: int) -> None:
    stat = path.stat()
    connection.execute("INSERT OR REPLACE INTO packages VALUES(?,?,?,?,?,?,?)",
                       (relative, stat.st_size, stat.st_mtime_ns,
                        "%s:%d" % (VERSION, max_profile_bytes),
                        json.dumps(result[0], ensure_ascii=False),
                        json.dumps(result[1], ensure_ascii=False), result[2]))


def discover_pptx(root: Path) -> list[Path]:
    paths = []
    for directory, dirs, files in os.walk(root, followlinks=False):
        dirs[:] = [name for name in dirs if not (Path(directory) / name).is_symlink()]
        for name in files:
            path = Path(directory) / name
            if path.suffix.casefold() == ".pptx" and not path.is_symlink():
                paths.append(path)
    return sorted(paths, key=lambda p: str(p).casefold())


def run_scan(root: Path, out: Path, max_profile_bytes: int,
             use_cache: bool, progress_every: int = 50) -> dict[str, Any]:
    root = root.resolve()
    out = out.resolve()
    if not root.is_dir():
        raise ValueError("input root does not exist or is not a directory: %s" % root)
    started = time.time()
    started_utc = datetime.now(timezone.utc).isoformat()
    paths = discover_pptx(root)
    assets: list[dict[str, Any]] = []
    issues: list[dict[str, str]] = []
    entries = 0
    cached_count = 0
    connection = _cache_open(out / "profiler-cache.sqlite3") if use_cache else None
    try:
        for index, path in enumerate(paths, 1):
            relative = path.relative_to(root).as_posix()
            result = _cached(connection, relative, path, max_profile_bytes) if connection else None
            if result is not None:
                cached_count += 1
            else:
                result = scan_package(path, relative, max_profile_bytes)
                if connection:
                    _cache_put(connection, relative, path, result, max_profile_bytes)
            assets.extend(result[0])
            issues.extend(result[1])
            entries += result[2]
            if progress_every > 0 and (index % progress_every == 0 or index == len(paths)):
                print("[%d/%d] PPTX scanned, %d JXR occurrences" %
                      (index, len(paths), len(assets)), file=sys.stderr)
        if connection:
            connection.commit()
    finally:
        if connection:
            connection.close()
    write_reports(out, root, assets, issues, len(paths), entries, cached_count,
                  started, datetime.now(timezone.utc).isoformat())
    return {"pptx_files": len(paths), "entries": entries, "assets": len(assets),
            "unique_assets": len({item["sha256"] for item in assets}),
            "issues": len(issues), "cached": cached_count, "out": str(out)}


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Profile JPEG XR streams inside PPTX files.")
    parser.add_argument("--root", required=True, type=Path, help="root folder to search recursively")
    parser.add_argument("--out", required=True, type=Path, help="report and SQLite cache folder")
    parser.add_argument("--max-entry-mb", type=int, default=512,
                        help="maximum uncompressed JXR entry size (default: 512 MiB)")
    parser.add_argument("--no-cache", action="store_true", help="rescan all PPTX files")
    parser.add_argument("--progress-every", type=int, default=50,
                        help="print progress every N PPTX files; 0 disables")
    args = parser.parse_args(argv)
    if args.max_entry_mb <= 0 or args.progress_every < 0:
        parser.error("limits must be positive and progress interval nonnegative")
    try:
        result = run_scan(args.root, args.out, args.max_entry_mb * 1024 * 1024,
                          not args.no_cache, args.progress_every)
    except (OSError, ValueError, sqlite3.Error) as exc:
        print("error: %s" % exc, file=sys.stderr)
        return 2
    print(json.dumps(result, ensure_ascii=False, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
