import struct
import sys
import tempfile
import unittest
import hashlib
import json
import zipfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import corpus


def make_bmp(width, height, bits, rows, palette=b"", top_down=False):
    dib_size = 40
    offset = 14 + dib_size + len(palette)
    stride = ((width * bits + 31) // 32) * 4
    file_size = offset + stride * height
    signed_height = -height if top_down else height
    header = b"BM" + struct.pack("<IHHI", file_size, 0, 0, offset)
    dib = struct.pack("<IiiHHIIiiII", dib_size, width, signed_height,
                      1, bits, 0, stride * height, 0, 0,
                      len(palette) // 4 if bits == 8 else 0, 0)
    encoded_rows = rows if top_down else list(reversed(rows))
    encoded = b"".join(row + bytes(stride - len(row)) for row in encoded_rows)
    return header + dib + palette + encoded


class CorpusUtilityTests(unittest.TestCase):
    def test_profile_representatives_and_all(self):
        first = {"sha256": "a", "parse_status": "parsed", "profile": {"overlap": 0}}
        duplicate_profile = {"sha256": "b", "parse_status": "parsed",
                             "profile": {"overlap": 0}}
        second_profile = {"sha256": "c", "parse_status": "parsed",
                          "profile": {"overlap": 1}}
        range_variant = {"sha256": "e", "parse_status": "parsed",
                         "profile": {"overlap": 0,
                                     "alpha_range_tag_value": 24576,
                                     "alpha_byte_count": 24576}}
        invalid = {"sha256": "d", "parse_status": "parse_error", "profile": {}}
        reps = corpus.select_assets([duplicate_profile, first, second_profile,
                                     range_variant, invalid], "representatives")
        self.assertEqual(["a", "c"], [item["sha256"] for item in reps])
        self.assertEqual(["a", "b", "c", "e"], [item["sha256"] for item in
                                               corpus.select_assets(
                                                   [duplicate_profile, first,
                                                    second_profile, range_variant,
                                                    invalid], "all")])

    def test_write_corpus_manifest_contains_index_not_asset_data(self):
        asset = {"sha256": "abc", "pptx": "folder/sample.pptx",
                 "entry": "ppt/media/image.wdp", "occurrences": 2,
                 "presentations": ["folder/sample.pptx", "copy.pptx"],
                 "parse_status": "parsed", "profile": {"width": 16},
                 "compressed_bytes": 42}
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / "manifest.jsonl"
            occurrences = [asset, {"sha256": "abc", "pptx": "copy.pptx",
                                  "entry": "ppt/media/copy.wdp"}]
            corpus.write_corpus_manifest([asset], path, occurrences)
            row = json.loads(path.read_text(encoding="utf-8"))
            same = corpus.compare_corpus_manifest(path, [asset], occurrences)
            modified = dict(asset, profile={"width": 17})
            changed = corpus.compare_corpus_manifest(path, [modified], occurrences)
        self.assertEqual("abc", row["sha256"])
        self.assertEqual("folder/sample.pptx", row["pptx"])
        self.assertEqual(2, row["occurrences"])
        self.assertEqual(2, len(row["locations"]))
        self.assertEqual({"width": 16}, row["profile"])
        self.assertNotIn("compressed_bytes", row)
        self.assertEqual({"assets_removed": 0, "assets_added": 0,
                          "assets_changed": 0}, same)
        self.assertEqual(1, changed["assets_changed"])

    def test_read_bottom_up_bgr24_and_padding(self):
        bmp = make_bmp(1, 2, 24, [bytes((1, 2, 3)), bytes((4, 5, 6))])
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / "pixels.bmp"
            path.write_bytes(bmp)
            width, height, channels, pixels = corpus.read_bmp(path)
        self.assertEqual((1, 2, 3), (width, height, channels))
        self.assertEqual(bytes((1, 2, 3, 4, 5, 6)), pixels)

    def test_read_top_down_bgra32(self):
        row = bytes((3, 2, 1, 255, 6, 5, 4, 127))
        bmp = make_bmp(2, 1, 32, [row], top_down=True)
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / "pixels.bmp"
            path.write_bytes(bmp)
            width, height, channels, pixels = corpus.read_bmp(path)
        self.assertEqual((2, 1, 4), (width, height, channels))
        self.assertEqual(row, pixels)

    def test_read_grayscale_palette_bmp(self):
        palette = b"".join(bytes((value, value, value, 0)) for value in range(256))
        bmp = make_bmp(2, 1, 8, [bytes((11, 207))], palette=palette)
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / "pixels.bmp"
            path.write_bytes(bmp)
            width, height, channels, pixels = corpus.read_bmp(path)
        self.assertEqual((2, 1, 1), (width, height, channels))
        self.assertEqual(bytes((11, 207)), pixels)

    def test_first_difference_reports_pixel_and_channel(self):
        result = corpus.first_difference(bytes((0, 1, 2, 3, 4, 5)),
                                         bytes((0, 1, 2, 3, 9, 5)), 2, 1, 3)
        self.assertEqual({"x": 1, "y": 0, "channel": 1,
                          "expected": 4, "actual": 9}, result)
        self.assertIsNone(corpus.first_difference(b"same", b"same", 2, 1, 2))

    def test_real_fixture_through_native_and_managed_decoders(self):
        repository = Path(__file__).resolve().parents[3]
        native = repository / "jxrencoderdecoder" / "Release" / "JXRDecApp" / "x64" / "JXRDecApp.exe"
        managed = repository / "managed" / "Jxr.Managed.CorpusRunner" / "bin" / "Release" / "Jxr.Managed.CorpusRunner.exe"
        fixture = repository / "real-image-profile" / "test-sign-334x330.jxr"
        alpha_fixture = repository / "managed" / "fixtures" / "alpha-planar-q1-32x32.jxr"
        if not native.is_file() or not managed.is_file():
            self.skipTest("build the native and managed corpus decoders to run this integration test")

        profiler_directory = repository / "tools" / "pptx-jxr-profiler"
        sys.path.insert(0, str(profiler_directory))
        import profiler

        package_name = "sample.pptx"
        entries = (("ppt/media/image1.jxr", fixture.read_bytes()),
                   ("ppt/media/image2.jxr", alpha_fixture.read_bytes()))
        assets = []
        for entry_name, jxr in entries:
            assets.append({"pptx": package_name, "entry": entry_name,
                           "sha256": hashlib.sha256(jxr).hexdigest(),
                           "parse_status": "parsed",
                           "profile": profiler.parse_jxr(jxr),
                           "presentations": [package_name]})
        with tempfile.TemporaryDirectory() as temp:
            temp_path = Path(temp)
            root = temp_path / "pptx"
            root.mkdir()
            with zipfile.ZipFile(root / package_name, "w",
                                 compression=zipfile.ZIP_DEFLATED) as package:
                for entry_name, jxr in entries:
                    package.writestr(entry_name, jxr)
            report = temp_path / "report"
            report.mkdir()
            (report / "assets.jsonl").write_text(
                "".join(json.dumps(asset) + "\n" for asset in assets),
                encoding="utf-8")
            (report / "occurrences.json").write_text(
                json.dumps(assets), encoding="utf-8")
            result_path = report / "corpus-results.json"
            reference_path = temp_path / "native-reference.jsonl"
            result = corpus.main([
                "--root", str(root), "--report-dir", str(report),
                "--native-decoder", str(native), "--managed-runner", str(managed),
                "--output", str(result_path), "--suite", "all",
                "--record-reference", str(reference_path),
            ])
            self.assertEqual(0, result, result_path.read_text(encoding="utf-8"))
            self.assertEqual(3, len(reference_path.read_text(
                encoding="utf-8").splitlines()))
            result_json = json.loads(result_path.read_text(encoding="utf-8"))
            self.assertEqual(2, result_json["assets_match"])
            self.assertEqual(1, result_json["presentation_coverage"][
                "fully_decoded_presentations"])
            result = corpus.main([
                "--root", str(root), "--report-dir", str(report),
                "--native-decoder", str(native), "--managed-runner", str(managed),
                "--output", str(result_path), "--suite", "all",
                "--reference-manifest", str(reference_path),
            ])
            self.assertEqual(0, result)
            profile_manifest = temp_path / "managed-profile-v2.jsonl"
            profile_result = corpus.main([
                "--root", str(root), "--report-dir", str(report),
                "--managed-runner", str(managed), "--output", str(result_path),
                "--suite", "all", "--profile-only",
                "--profile-manifest", str(profile_manifest),
            ])
            self.assertEqual(0, profile_result,
                             result_path.read_text(encoding="utf-8"))
            profile_summary = json.loads(result_path.read_text(encoding="utf-8"))
            self.assertEqual(2, profile_summary["profiles_match"])
            self.assertEqual(2, len(profile_manifest.read_text(
                encoding="utf-8").splitlines()))


if __name__ == "__main__":
    unittest.main()
