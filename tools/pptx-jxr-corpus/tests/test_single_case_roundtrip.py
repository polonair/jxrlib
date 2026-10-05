import sys
import tempfile
import unittest
import zipfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import single_case_roundtrip as step9


class SingleCaseRoundTripTests(unittest.TestCase):
    def test_pinned_example_is_one_supported_non_mismatch_profile(self):
        repository = Path(__file__).resolve().parents[3]
        row = step9.read_manifest_profile(
            repository / "tools" / "pptx-jxr-corpus" / "corpus-manifest.jsonl")
        step9.assert_expected_profile(row["profile"])
        self.assertEqual(step9.CASE_ENTRY, row["entry"])
        self.assertEqual(step9.CASE_SHA256, row["sha256"])
        for name in ("known-mismatches-frequency-no-alpha.jsonl",
                     "known-mismatches-frequency-planar-alpha.jsonl",
                     "known-mismatches-spatial.jsonl"):
            path = repository / "tools" / "pptx-jxr-corpus" / name
            self.assertNotIn(step9.CASE_SHA256, path.read_text(encoding="utf-8"))

    def test_replacement_changes_only_selected_package_member(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            source = root / "source.pptx"
            output = root / "output.pptx"
            with zipfile.ZipFile(source, "w",
                                 compression=zipfile.ZIP_DEFLATED) as package:
                package.comment = b"step9 test package"
                package.writestr("[Content_Types].xml", b"types")
                package.writestr("ppt/slides/slide1.xml", b"slide")
                package.writestr("ppt/media/image.wdp", b"old-jxr")

            result = step9.replace_zip_entry(source, output,
                                             "ppt/media/image.wdp", b"new-jxr")
            self.assertEqual(3, result["zip_entries"])
            self.assertEqual(2, result["unchanged_entries"])
            with zipfile.ZipFile(source, "r") as original, \
                    zipfile.ZipFile(output, "r") as modified:
                self.assertIsNone(modified.testzip())
                self.assertEqual(original.namelist(), modified.namelist())
                self.assertEqual(original.comment, modified.comment)
                self.assertEqual(b"new-jxr", modified.read("ppt/media/image.wdp"))
                for name in ("[Content_Types].xml", "ppt/slides/slide1.xml"):
                    self.assertEqual(original.read(name), modified.read(name))

    def test_existing_output_is_never_overwritten(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            source = root / "source.pptx"
            output = root / "output.pptx"
            with zipfile.ZipFile(source, "w") as package:
                package.writestr("ppt/media/image.wdp", b"old")
            output.write_bytes(b"preserve me")
            with self.assertRaises(FileExistsError):
                step9.replace_zip_entry(source, output,
                                        "ppt/media/image.wdp", b"new")
            self.assertEqual(b"preserve me", output.read_bytes())


if __name__ == "__main__":
    unittest.main()
