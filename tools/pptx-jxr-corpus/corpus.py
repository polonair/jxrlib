#!/usr/bin/env python3
"""Compare managed JPEG XR decoding with the native decoder on PPTX assets."""

from __future__ import annotations

import argparse
import hashlib
import json
import struct
import subprocess
import sys
import tempfile
import zipfile
from pathlib import Path
from typing import Any


SCHEMA_VERSION = 1
CHANNELS = ("color", "alpha")


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def profile_key(asset: dict[str, Any]) -> str:
    """Stable key for choosing one representative of each parsed profile."""
    profile = asset.get("profile") or {}
    fields = (
        "source_color_format", "source_bit_depth", "orientation", "overlap",
        "has_alpha", "alpha_mode", "bitstream_layout", "tile_mode",
        "tile_columns", "tile_rows", "subbands", "scaled_arithmetic",
        "sample_conversion", "shift_or_mantissa", "channel_count",
        "frame_quantizers", "pixel_format_guid", "alpha_range_interpretation",
    )
    return json.dumps({key: profile.get(key) for key in fields},
                      sort_keys=True, separators=(",", ":"))


def select_assets(assets: list[dict[str, Any]], suite: str) -> list[dict[str, Any]]:
    parsed = [asset for asset in assets
              if asset.get("parse_status") == "parsed" and asset.get("sha256")]
    parsed.sort(key=lambda asset: asset["sha256"])
    if suite == "all":
        return parsed
    chosen: dict[str, dict[str, Any]] = {}
    for asset in parsed:
        chosen.setdefault(profile_key(asset), asset)
    return list(chosen.values())


def read_bmp(path: Path) -> tuple[int, int, int, bytes]:
    """Return width, height, channels and top-down tightly packed pixels."""
    data = path.read_bytes()
    if len(data) < 54 or data[:2] != b"BM":
        raise ValueError("native decoder output is not a BMP")
    pixel_offset = struct.unpack_from("<I", data, 10)[0]
    dib_size = struct.unpack_from("<I", data, 14)[0]
    if dib_size < 40 or len(data) < 14 + dib_size:
        raise ValueError("unsupported BMP header")
    width, signed_height = struct.unpack_from("<ii", data, 18)
    planes, bits, compression = struct.unpack_from("<HHI", data, 26)
    if width <= 0 or signed_height == 0 or planes != 1 or compression != 0:
        raise ValueError("unsupported BMP dimensions or compression")
    height = abs(signed_height)
    if bits not in (8, 24, 32):
        raise ValueError("unsupported BMP pixel depth: %d" % bits)
    row_stride = ((width * bits + 31) // 32) * 4
    if pixel_offset + row_stride * height > len(data):
        raise ValueError("truncated BMP pixels")
    channels = bits // 8
    if bits == 8:
        colors_used = struct.unpack_from("<I", data, 46)[0]
        palette_size = colors_used or 256
        palette_start = 14 + dib_size
        palette_end = palette_start + palette_size * 4
        if palette_end > pixel_offset:
            raise ValueError("truncated BMP palette")
        palette = data[palette_start:pixel_offset]
        pixels = bytearray(width * height)
    else:
        pixels = bytearray(width * height * channels)
    for y in range(height):
        source_y = y if signed_height < 0 else height - y - 1
        row_start = pixel_offset + source_y * row_stride
        target_start = y * width * channels
        row = data[row_start:row_start + width * channels]
        if bits == 8:
            for x, index in enumerate(row):
                palette_index = index * 4
                if palette_index + 3 > len(palette):
                    raise ValueError("BMP palette index out of range")
                blue, green, red = palette[palette_index:palette_index + 3]
                if blue != green or green != red:
                    raise ValueError("native grayscale BMP has a non-gray palette")
                pixels[y * width + x] = blue
        else:
            pixels[target_start:target_start + len(row)] = row
    return width, height, channels, bytes(pixels)


def first_difference(expected: bytes, actual: bytes, width: int,
                     height: int, channels: int) -> dict[str, int] | None:
    if len(expected) != len(actual):
        return {"byte_offset": min(len(expected), len(actual)),
                "expected_length": len(expected), "actual_length": len(actual)}
    for offset, (left, right) in enumerate(zip(expected, actual)):
        if left != right:
            pixel = offset // channels
            return {"x": pixel % width, "y": pixel // width,
                    "channel": offset % channels,
                    "expected": left, "actual": right}
    return None


def load_assets(report_dir: Path) -> list[dict[str, Any]]:
    path = report_dir / "assets.jsonl"
    if not path.is_file():
        raise FileNotFoundError("profiler report missing: " + str(path))
    assets = []
    with path.open("r", encoding="utf-8") as stream:
        for line in stream:
            if line.strip():
                assets.append(json.loads(line))
    return assets


def load_occurrences(report_dir: Path) -> list[dict[str, Any]]:
    path = report_dir / "occurrences.json"
    if not path.is_file():
        return []
    return json.loads(path.read_text(encoding="utf-8"))


def make_corpus_manifest(assets: list[dict[str, Any]],
                         occurrences: list[dict[str, Any]]) -> list[dict[str, Any]]:
    locations_by_hash: dict[str, list[dict[str, str]]] = {}
    for occurrence in occurrences:
        locations_by_hash.setdefault(occurrence.get("sha256", ""), []).append({
            "pptx": occurrence.get("pptx", ""),
            "entry": occurrence.get("entry", ""),
        })
    rows = []
    for asset in sorted(assets, key=lambda value: value.get("sha256", "")):
        locations = locations_by_hash.get(asset.get("sha256", ""), [
            {"pptx": asset.get("pptx", ""), "entry": asset.get("entry", "")}
        ])
        locations.sort(key=lambda value: (value["pptx"], value["entry"]))
        rows.append({
            "schema_version": SCHEMA_VERSION,
            "sha256": asset.get("sha256"),
            "parse_status": asset.get("parse_status"),
            "parse_error": asset.get("parse_error", ""),
            "pptx": asset.get("pptx"),
            "entry": asset.get("entry"),
            "occurrences": asset.get("occurrences", 1),
            "presentations": asset.get("presentations", [asset.get("pptx")]),
            "locations": locations,
            "profile": profile_summary(asset),
        })
    return rows


def write_corpus_manifest(assets: list[dict[str, Any]], path: Path,
                          occurrences: list[dict[str, Any]]) -> None:
    """Write a data-free index of every unique JXR and its PPTX locations."""
    rows = make_corpus_manifest(assets, occurrences)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("".join(json.dumps(row, sort_keys=True, ensure_ascii=False) + "\n"
                                  for row in rows), encoding="utf-8")


def compare_corpus_manifest(expected_path: Path, assets: list[dict[str, Any]],
                            occurrences: list[dict[str, Any]]) -> dict[str, int]:
    expected_rows = []
    with expected_path.open("r", encoding="utf-8") as stream:
        expected_rows = [json.loads(line) for line in stream if line.strip()]
    expected = {row["sha256"]: row for row in expected_rows}
    current_rows = make_corpus_manifest(assets, occurrences)
    current = {row["sha256"]: row for row in current_rows}
    removed = set(expected) - set(current)
    added = set(current) - set(expected)
    changed = {key for key in set(expected) & set(current)
               if any(expected[key].get(field) != current[key].get(field)
                      for field in ("parse_status", "parse_error", "pptx", "entry",
                                    "occurrences", "presentations", "locations", "profile"))}
    return {"assets_removed": len(removed), "assets_added": len(added),
            "assets_changed": len(changed)}


def profile_summary(asset: dict[str, Any]) -> dict[str, Any]:
    return dict(asset.get("profile") or {})


def run_process(command: list[str], cwd: Path) -> tuple[int, str]:
    result = subprocess.run(command, cwd=str(cwd), stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, universal_newlines=True)
    return result.returncode, result.stdout.strip()


def native_pixels(decoder: Path, jxr_path: Path, work: Path,
                  channel: str) -> tuple[int, int, int, bytes]:
    bmp_path = work / (channel + ".bmp")
    command = [str(decoder), "-i", str(jxr_path), "-o", str(bmp_path)]
    if channel == "alpha":
        command.extend(("-c", "2", "-a", "1"))
    else:
        command.extend(("-c", "0", "-a", "0"))
    command.extend(("-p", "0"))
    code, output = run_process(command, decoder.parent)
    if code != 0 or not bmp_path.is_file():
        raise RuntimeError("native decoder failed (%d): %s" % (code, output))
    width, height, channels, pixels = read_bmp(bmp_path)
    expected_channels = 1 if channel == "alpha" else 3
    if channels != expected_channels:
        raise ValueError("native decoder returned %d channels for %s" %
                         (channels, channel))
    return width, height, channels, pixels


def native_color_and_alpha(decoder: Path, jxr_path: Path,
                           work: Path) -> tuple[tuple[int, int, int, bytes],
                                                tuple[int, int, int, bytes]]:
    bmp_path = work / "native-bgra.bmp"
    command = [str(decoder), "-i", str(jxr_path), "-o", str(bmp_path),
               "-c", "22", "-a", "2", "-p", "0"]
    code, output = run_process(command, decoder.parent)
    if code != 0 or not bmp_path.is_file():
        raise RuntimeError("native decoder failed (%d): %s" % (code, output))
    width, height, channels, bgra = read_bmp(bmp_path)
    if channels != 4:
        raise ValueError("native alpha decode did not return BGRA32")
    color = bytearray(width * height * 3)
    alpha = bytearray(width * height)
    for pixel in range(width * height):
        color_offset = pixel * 3
        bgra_offset = pixel * 4
        color[color_offset:color_offset + 3] = bgra[bgra_offset:bgra_offset + 3]
        alpha[pixel] = bgra[bgra_offset + 3]
    return ((width, height, 3, bytes(color)),
            (width, height, 1, bytes(alpha)))


def managed_pixels(runner: Path, jxr_path: Path, work: Path,
                   channel: str) -> tuple[int, int, int, bytes]:
    raw_path = work / ("managed-" + channel + ".bin")
    metadata_path = work / ("managed-" + channel + ".txt")
    command = [str(runner), "decode", str(jxr_path), channel,
               str(raw_path), str(metadata_path)]
    code, output = run_process(command, runner.parent)
    if code != 0 or not raw_path.is_file() or not metadata_path.is_file():
        raise RuntimeError("managed decoder failed (%d): %s" % (code, output))
    metadata = {}
    for line in metadata_path.read_text(encoding="utf-8-sig").splitlines():
        if "=" in line:
            key, value = line.split("=", 1)
            metadata[key] = value
    width, height, stride = (int(metadata[key])
                             for key in ("width", "height", "stride"))
    channels = 1 if channel == "alpha" else 3
    packed_stride = width * channels
    raw = raw_path.read_bytes()
    if width <= 0 or height <= 0 or stride < packed_stride or len(raw) != stride * height:
        raise ValueError("managed decoder returned inconsistent dimensions/buffer")
    if stride == packed_stride:
        pixels = raw
    else:
        pixels = b"".join(raw[row * stride:row * stride + packed_stride]
                          for row in range(height))
    return width, height, channels, pixels


def native_reference(decoder: Path, jxr: bytes, work: Path,
                     channels: tuple[str, ...]) -> dict[str, dict[str, Any]]:
    jxr_path = work / "source.jxr"
    jxr_path.write_bytes(jxr)
    result = {}
    if channels == ("color", "alpha"):
        planes = native_color_and_alpha(decoder, jxr_path, work)
        for channel, plane in zip(channels, planes):
            width, height, count, pixels = plane
            result[channel] = {"width": width, "height": height,
                               "channels": count, "pixels": pixels,
                               "sha256": digest(pixels)}
        return result
    for channel in channels:
        width, height, count, pixels = native_pixels(decoder, jxr_path, work,
                                                     channel)
        result[channel] = {"width": width, "height": height,
                           "channels": count, "pixels": pixels,
                           "sha256": digest(pixels)}
    return result


def make_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", required=True, type=Path,
                        help="PPTX corpus root")
    parser.add_argument("--report-dir", required=True, type=Path,
                        help="PPTX profiler report/cache directory")
    parser.add_argument("--native-decoder", type=Path,
                        help="built JXRDecApp.exe")
    parser.add_argument("--managed-runner", type=Path,
                        help="built Jxr.Managed.CorpusRunner.exe")
    parser.add_argument("--output", type=Path,
                        help="JSON results path; defaults to report-dir/corpus-results.json")
    parser.add_argument("--suite", choices=("representatives", "all"),
                        default="representatives")
    parser.add_argument("--refresh-inventory", action="store_true",
                        help="run the profiler before loading its assets.jsonl")
    parser.add_argument("--manifest-only", action="store_true",
                        help="write the corpus index without invoking decoders")
    parser.add_argument("--write-corpus-manifest", type=Path,
                        help="write the full path/SHA/profile index of unique JXR files")
    parser.add_argument("--corpus-manifest", type=Path,
                        help="verify the profiler inventory against a saved corpus index")
    parser.add_argument("--record-reference", type=Path,
                        help="write native pixel digests for the selected suite")
    parser.add_argument("--reference-manifest", type=Path,
                        help="compare native output with a saved reference manifest")
    parser.add_argument("--diagnostic", action="store_true",
                        help="exit successfully when processing completes, even with managed mismatches")
    parser.add_argument("--max-entry-mb", type=int, default=512)
    return parser


def main(argv: list[str] | None = None) -> int:
    args = make_parser().parse_args(argv)
    root = args.root.resolve()
    report_dir = args.report_dir.resolve()
    decoder = args.native_decoder.resolve() if args.native_decoder else None
    runner = args.managed_runner.resolve() if args.managed_runner else None
    output_path = (args.output or report_dir / "corpus-results.json").resolve()
    required_paths = [(root, "corpus root")]
    if not args.manifest_only:
        if decoder is None or runner is None:
            print("--native-decoder and --managed-runner are required for decoding",
                  file=sys.stderr)
            return 2
        required_paths.extend(((decoder, "native decoder"),
                               (runner, "managed runner")))
    for path, label in required_paths:
        if not path.exists():
            print("Missing %s: %s" % (label, path), file=sys.stderr)
            return 2
    if args.max_entry_mb <= 0:
        print("--max-entry-mb must be positive", file=sys.stderr)
        return 2

    if args.refresh_inventory:
        profiler = Path(__file__).resolve().parents[1] / "pptx-jxr-profiler" / "profiler.py"
        code, text = run_process([sys.executable, str(profiler), "--root",
                                  str(root), "--out", str(report_dir),
                                  "--max-entry-mb", str(args.max_entry_mb)],
                                 profiler.parent)
        if text:
            print(text)
        if code != 0:
            return code

    try:
        inventory = load_assets(report_dir)
        occurrences = load_occurrences(report_dir)
        assets = select_assets(inventory, args.suite)
    except (OSError, ValueError, json.JSONDecodeError) as error:
        print(str(error), file=sys.stderr)
        return 2
    if args.write_corpus_manifest:
        write_corpus_manifest(inventory, args.write_corpus_manifest.resolve(),
                              occurrences)
        print("Wrote %d unique JXR index rows: %s" % (
            len(inventory), args.write_corpus_manifest.resolve()))
    manifest_check = {"assets_removed": 0, "assets_added": 0,
                      "assets_changed": 0}
    if args.corpus_manifest:
        try:
            manifest_check = compare_corpus_manifest(
                args.corpus_manifest.resolve(), inventory, occurrences)
        except (OSError, ValueError, KeyError, json.JSONDecodeError) as error:
            print("Invalid corpus manifest: " + str(error), file=sys.stderr)
            return 2
        print("Corpus manifest differences: %s" % manifest_check)
    if args.manifest_only:
        return 0 if not any(manifest_check.values()) else 1
    if not assets:
        print("No parsed JXR assets selected", file=sys.stderr)
        return 2

    references = []
    results = []
    observed_reference_keys = set()
    reference_lookup = {}
    if args.reference_manifest:
        try:
            with args.reference_manifest.open("r", encoding="utf-8") as stream:
                for line in stream:
                    if line.strip():
                        reference = json.loads(line)
                        reference_lookup[(reference["sha256"],
                                          reference["channel"])] = reference
        except (OSError, ValueError, KeyError, json.JSONDecodeError) as error:
            print("Invalid reference manifest: " + str(error), file=sys.stderr)
            return 2
    for index, asset in enumerate(assets, 1):
        relative_pptx = Path(asset["pptx"])
        pptx = (root / relative_pptx).resolve()
        record: dict[str, Any] = {
            "sha256": asset["sha256"], "pptx": asset["pptx"],
            "entry": asset["entry"], "profile": profile_summary(asset),
            "channels": [], "status": "pending",
        }
        print("[%d/%d] %s :: %s" % (index, len(assets), asset["pptx"],
                                     asset["entry"]), flush=True)
        if not pptx.is_relative_to(root):
            record["status"] = "invalid_manifest_path"
            record["error"] = "PPTX path escapes corpus root"
            results.append(record)
            continue
        try:
            with zipfile.ZipFile(pptx, "r") as package:
                info = package.getinfo(asset["entry"])
                if info.file_size > args.max_entry_mb * 1024 * 1024:
                    raise ValueError("entry exceeds configured size limit")
                with package.open(info, "r") as source:
                    with tempfile.TemporaryDirectory(prefix="jxr-corpus-") as temp:
                        work = Path(temp)
                        jxr = source.read(args.max_entry_mb * 1024 * 1024 + 1)
                        if len(jxr) > args.max_entry_mb * 1024 * 1024:
                            raise ValueError("entry exceeds configured size limit")
                        if digest(jxr) != asset["sha256"]:
                            raise ValueError("asset SHA-256 differs from profiler report")
                        profile = asset.get("profile") or {}
                        has_alpha = bool(profile.get("has_alpha")) or \
                            profile.get("alpha_mode") not in (None, "none")
                        channels = ("color", "alpha") if has_alpha else ("color",)
                        jxr_path = work / "source.jxr"
                        jxr_path.write_bytes(jxr)
                        reference_data = native_reference(decoder, jxr, work,
                                                          channels)
                        channel_results = []
                        for channel in channels:
                            ref = reference_data[channel]
                            references.append({
                                "schema_version": SCHEMA_VERSION,
                                "sha256": asset["sha256"],
                                "pptx": asset["pptx"],
                                "entry": asset["entry"],
                                "presentations": asset.get("presentations", [asset["pptx"]]),
                                "profile": profile_summary(asset),
                                "channel": channel,
                                "width": ref["width"], "height": ref["height"],
                                "channels": ref["channels"],
                                "native_pixel_sha256": ref["sha256"],
                            })
                            channel_result = {"channel": channel,
                                "native_sha256": ref["sha256"]}
                            saved_reference = reference_lookup.get(
                                (asset["sha256"], channel))
                            observed_reference_keys.add((asset["sha256"], channel))
                            if args.reference_manifest and saved_reference is None:
                                channel_result["reference_missing"] = True
                            if saved_reference is not None and (
                                saved_reference.get("native_pixel_sha256") != ref["sha256"] or
                                saved_reference.get("width") != ref["width"] or
                                saved_reference.get("height") != ref["height"] or
                                saved_reference.get("channels") != ref["channels"] or
                                saved_reference.get("profile") != profile_summary(asset)):
                                channel_result["reference_drift"] = True
                            try:
                                width, height, count, actual = managed_pixels(
                                    runner, jxr_path, work, channel)
                                if (width, height, count) != (ref["width"],
                                    ref["height"], ref["channels"]):
                                    difference = {"expected_width": ref["width"],
                                        "expected_height": ref["height"],
                                        "actual_width": width,
                                        "actual_height": height,
                                        "expected_channels": ref["channels"],
                                        "actual_channels": count}
                                else:
                                    difference = first_difference(ref["pixels"],
                                        actual, width, height, count)
                                channel_result["status"] = ("match" if difference is None
                                    else "pixel_mismatch")
                                channel_result["managed_sha256"] = digest(actual)
                                channel_result["difference"] = difference
                            except (OSError, RuntimeError, ValueError, KeyError) as error:
                                channel_result["status"] = "managed_decode_error"
                                channel_result["error"] = str(error)
                            channel_results.append(channel_result)
                        record["channels"] = channel_results
                        if any(item.get("reference_drift") or
                               item.get("reference_missing")
                               for item in channel_results):
                            record["status"] = "reference_drift"
                        elif any(item["status"] == "managed_decode_error"
                                 for item in channel_results):
                            record["status"] = "managed_decode_error"
                        elif any(item["status"] == "pixel_mismatch"
                                 for item in channel_results):
                            record["status"] = "pixel_mismatch"
                        else:
                            record["status"] = "match"
        except (OSError, RuntimeError, ValueError, KeyError,
                zipfile.BadZipFile, struct.error) as error:
            record["status"] = "error"
            record["error"] = str(error)
        results.append(record)

    output_path.parent.mkdir(parents=True, exist_ok=True)
    presentation_coverage = None
    if args.suite == "all" and occurrences:
        status_by_hash = {row["sha256"]: row["status"] for row in results}
        hashes_by_presentation: dict[str, set[str]] = {}
        for occurrence in occurrences:
            hashes_by_presentation.setdefault(occurrence["pptx"], set()).add(
                occurrence["sha256"])
        covered = [pptx for pptx, hashes in hashes_by_presentation.items()
                   if all(status_by_hash.get(value) == "match" for value in hashes)]
        presentation_coverage = {
            "presentations_with_jxr": len(hashes_by_presentation),
            "fully_decoded_presentations": len(covered),
            "not_fully_decoded_presentations": len(hashes_by_presentation) - len(covered),
        }
    missing_references = 0
    untracked_assets = 0
    if args.reference_manifest and args.suite == "all":
        missing_references = len(set(reference_lookup) - observed_reference_keys)
        untracked_assets = len(observed_reference_keys - set(reference_lookup))
    summary = {
        "schema_version": SCHEMA_VERSION,
        "suite": args.suite,
        "corpus_root": str(root),
        "inventory_unique_assets": len(inventory),
        "inventory_parse_errors": sum(item.get("parse_status") != "parsed"
                                       for item in inventory),
        "assets_selected": len(assets),
        "assets_match": sum(row["status"] == "match" for row in results),
        "assets_mismatch": sum(row["status"] == "pixel_mismatch" for row in results),
        "assets_error": sum(row["status"] in ("error", "invalid_manifest_path")
                            for row in results),
        "managed_decode_errors": sum(row["status"] == "managed_decode_error"
                                     for row in results),
        "reference_drift": sum(row["status"] == "reference_drift"
                                for row in results),
        "reference_missing": sum(any(channel.get("reference_missing")
                                     for channel in row.get("channels", []))
                                 for row in results),
        "reference_unmatched": missing_references,
        "reference_untracked": untracked_assets,
        "corpus_manifest": manifest_check if args.corpus_manifest else None,
        "reference_capture_complete": (args.record_reference is not None and
                                        not any(row["status"] == "error"
                                                for row in results)),
        "presentation_coverage": presentation_coverage,
        "results": results,
    }
    output_path.write_text(json.dumps(summary, ensure_ascii=False, indent=2) + "\n",
                           encoding="utf-8")
    if args.record_reference:
        reference_path = args.record_reference.resolve()
        if references:
            reference_path.parent.mkdir(parents=True, exist_ok=True)
            reference_path.write_text("".join(json.dumps(row, sort_keys=True,
                ensure_ascii=False) + "\n" for row in references), encoding="utf-8")
            print("Wrote %d native reference rows%s: %s" % (
                len(references), " (partial; see corpus results for native errors)"
                if not summary["reference_capture_complete"] else "", reference_path))
        else:
            print("No native references were produced", file=sys.stderr)
    print("Summary: %d/%d match, %d mismatches, %d managed decode errors, %d errors, %d reference drifts, %d missing references" % (
        summary["assets_match"], len(results), summary["assets_mismatch"],
        summary["managed_decode_errors"], summary["assets_error"],
        summary["reference_drift"], summary["reference_missing"]))
    if (summary["assets_error"] or summary["reference_drift"] or
            summary["reference_missing"] or summary["reference_untracked"]):
        return 1
    if summary["corpus_manifest"] and any(summary["corpus_manifest"].values()):
        return 1
    if summary["reference_unmatched"]:
        return 1
    if (summary["assets_mismatch"] or summary["managed_decode_errors"]) and not args.diagnostic:
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
