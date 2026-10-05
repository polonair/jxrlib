import json
import csv
import sys
import tempfile
import unittest
import zipfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import profiler


REPO_ROOT = Path(__file__).resolve().parents[3]
MINIMAL_JXR = REPO_ROOT / "minimal-profile" / "minimal-gray-16x16.jxr"
PLANAR_ALPHA_JXR = REPO_ROOT / "managed" / "fixtures" / "alpha-planar-q1-32x32.jxr"


def write_pptx(path, jxr):
    with zipfile.ZipFile(path, "w", compression=zipfile.ZIP_DEFLATED) as archive:
        archive.writestr("[Content_Types].xml", """<?xml version="1.0"?>
          <Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
            <Default Extension="hdr" ContentType="image/vnd.ms-photo"/>
            <Override PartName="/ppt/media/original.hdr" ContentType="image/vnd.ms-photo"/>
          </Types>""")
        archive.writestr("ppt/slides/_rels/slide1.xml.rels", """<?xml version="1.0"?>
          <Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
            <Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/image" Target="../media/original.hdr"/>
            <Relationship Id="rId2" Type="http://schemas.microsoft.com/office/word/2010/wordprocessingGroup" Target="../media/other.bin"/>
          </Relationships>""")
        archive.writestr("ppt/slides/slide1.xml", """<?xml version="1.0"?>
          <p:sld xmlns:p="urn:p" xmlns:a="urn:a" xmlns:r="urn:r" xmlns:a14="urn:a14">
            <a:blip r:embed="rId1"><a:extLst><a14:imgLayer r:embed="rId2"/></a:extLst></a:blip>
          </p:sld>""")
        archive.writestr("ppt/media/original.hdr", jxr)
        archive.writestr("ppt/media/other.bin", jxr)
        archive.writestr("ppt/media/radiance.hdr", b"#?RADIANCE\nFORMAT=32-bit_rle_rgbe\n")
        archive.writestr("ppt/media/broken.jxr", b"WMPHOTO\x00bad")


class ProfilerTests(unittest.TestCase):
    def test_parse_native_container_header(self):
        report = profiler.parse_jxr(MINIMAL_JXR.read_bytes())
        self.assertEqual(report["container"], "tiff_like_jxr")
        self.assertEqual(report["width"], 16)
        self.assertEqual(report["height"], 16)
        self.assertEqual(report["source_color_format"], "Y_ONLY")
        self.assertEqual(report["source_bit_depth"], "8bpp")
        self.assertEqual(report["pixel_format"], "Gray8")
        self.assertEqual(report["bitstream_layout"], "spatial")
        self.assertEqual(report["overlap"], 0)
        self.assertEqual(report["frame_quantizers"]["dc"]["effective_indices"], [0])
        self.assertEqual(report["tile_quantizer_scope"], "not_parsed")

    def test_parse_raw_codestream(self):
        payload = MINIMAL_JXR.read_bytes()
        container = profiler.parse_jxr(payload)
        start = container["codestream_offset"]
        end = start + container["codestream_length"]
        report = profiler.parse_codestream(payload[start:end])
        self.assertEqual(report["width"], 16)
        self.assertEqual(report["frame_header_parse_complete"], True)

    def test_parse_planar_alpha_range_as_offset_and_length(self):
        report = profiler.parse_jxr(PLANAR_ALPHA_JXR.read_bytes())
        self.assertEqual(report["alpha_mode"], "planar")
        self.assertEqual(report["alpha_end_offset"],
                         report["alpha_offset"] + report["alpha_byte_count"])
        self.assertEqual(report["alpha_range_interpretation"], "absolute_end_offset")
        self.assertEqual(report["alpha_profile"]["width"], report["width"])
        self.assertEqual(report["alpha_profile"]["height"], report["height"])

    def test_parse_planar_alpha_byte_count_variant(self):
        payload = bytearray(PLANAR_ALPHA_JXR.read_bytes())
        endian = "<" if payload[:2] == b"II" else ">"
        ifd = profiler.struct.unpack(endian + "I", payload[4:8])[0]
        count = profiler.struct.unpack(endian + "H", payload[ifd:ifd + 2])[0]
        alpha_offset = None
        alpha_end_entry = None
        for index in range(count):
            entry = ifd + 2 + index * 12
            tag = profiler.struct.unpack(endian + "H", payload[entry:entry + 2])[0]
            if tag == 0xbcc2:
                alpha_offset = profiler.struct.unpack(endian + "I", payload[entry + 8:entry + 12])[0]
            elif tag == 0xbcc3:
                alpha_end_entry = entry + 8
        self.assertIsNotNone(alpha_offset)
        self.assertIsNotNone(alpha_end_entry)
        alpha_end = profiler.struct.unpack(endian + "I", payload[alpha_end_entry:alpha_end_entry + 4])[0]
        profiler.struct.pack_into(endian + "I", payload, alpha_end_entry, alpha_end - alpha_offset)
        report = profiler.parse_jxr(bytes(payload))
        self.assertEqual(report["alpha_range_interpretation"], "byte_count")
        self.assertEqual(report["alpha_end_offset"], alpha_end)
        self.assertEqual(report["alpha_byte_count"], alpha_end - alpha_offset)

    def test_rejects_invalid_planar_alpha_ranges(self):
        original = PLANAR_ALPHA_JXR.read_bytes()
        endian = "<" if original[:2] == b"II" else ">"
        ifd = profiler.struct.unpack(endian + "I", original[4:8])[0]
        count = profiler.struct.unpack(endian + "H", original[ifd:ifd + 2])[0]
        alpha_range_entry = None
        for index in range(count):
            entry = ifd + 2 + index * 12
            tag = profiler.struct.unpack(endian + "H", original[entry:entry + 2])[0]
            if tag == 0xbcc3:
                alpha_range_entry = entry + 8
                break
        self.assertIsNotNone(alpha_range_entry)
        for invalid_value in (0, len(original) + 1):
            payload = bytearray(original)
            profiler.struct.pack_into(endian + "I", payload,
                                      alpha_range_entry, invalid_value)
            with self.assertRaises(profiler.ProfileError):
                profiler.parse_jxr(bytes(payload))

    def test_scans_hidden_extensions_relationships_and_errors(self):
        jxr = MINIMAL_JXR.read_bytes()
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp) / "input"
            out = Path(temp) / "report"
            root.mkdir()
            write_pptx(root / "example.PPTX", jxr)
            result = profiler.run_scan(root, out, 16 * 1024 * 1024, True, 0)
            self.assertEqual(result["pptx_files"], 1)
            self.assertEqual(result["assets"], 3)
            self.assertEqual(result["unique_assets"], 2)
            occurrence_rows = json.loads((out / "occurrences.json").read_text(encoding="utf-8"))
            by_entry = {row["entry"]: row for row in occurrence_rows}
            self.assertEqual(by_entry["ppt/media/original.hdr"]["profile"]["width"], 16)
            roles = by_entry["ppt/media/original.hdr"]["relationships"]
            self.assertEqual([role["role"] for role in roles], ["display_blip"])
            self.assertEqual(by_entry["ppt/media/other.bin"]["relationships"][0]["role"],
                             "original_imgLayer")
            issues = json.loads((out / "errors.json").read_text(encoding="utf-8"))
            self.assertTrue(any(item["stage"] == "signature" for item in issues))
            self.assertTrue(any(item["stage"] == "jxr_parse" for item in issues))
            jsonl = (out / "assets.jsonl").read_text(encoding="utf-8").splitlines()
            self.assertEqual(len(jsonl), 2)
            self.assertTrue(all(json.loads(line)["sha256"] for line in jsonl))
            with (out / "occurrences.csv").open(encoding="utf-8-sig", newline="") as handle:
                csv_rows = list(csv.DictReader(handle))
            self.assertTrue(json.loads(csv_rows[0]["profile"])["width"] == 16)

    def test_cache_reuses_unchanged_pptx_and_invalidates_changed_file(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp) / "input"
            out = Path(temp) / "report"
            root.mkdir()
            pptx = root / "same.pptx"
            write_pptx(pptx, MINIMAL_JXR.read_bytes())
            first = profiler.run_scan(root, out, 16 * 1024 * 1024, True, 0)
            second = profiler.run_scan(root, out, 16 * 1024 * 1024, True, 0)
            self.assertEqual(first["cached"], 0)
            self.assertEqual(second["cached"], 1)
            tighter = profiler.run_scan(root, out, 1, True, 0)
            self.assertEqual(tighter["cached"], 0)
            self.assertTrue(any(issue["stage"] == "jxr_read"
                                for issue in json.loads((out / "errors.json").read_text(encoding="utf-8"))))
            restored = profiler.run_scan(root, out, 16 * 1024 * 1024, True, 0)
            self.assertEqual(restored["cached"], 0)
            write_pptx(pptx, MINIMAL_JXR.read_bytes() + b"\x00")
            third = profiler.run_scan(root, out, 16 * 1024 * 1024, True, 0)
            self.assertEqual(third["cached"], 0)

    def test_malformed_header_is_reported(self):
        with self.assertRaises(profiler.ProfileError):
            profiler.parse_codestream(b"WMPHOTO\x00\x00")

    def test_feature_group_includes_frame_quantizer_syntax(self):
        report = profiler.parse_jxr(MINIMAL_JXR.read_bytes())
        group = json.loads(profiler.feature_profile(report))
        self.assertEqual(group["frame_quantizers"], json.dumps(
            report["frame_quantizers"], sort_keys=True, separators=(",", ":")))


if __name__ == "__main__":
    unittest.main()
