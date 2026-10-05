#!/usr/bin/env python3
"""Run the fixed Step 9 JXR round trip for one PPTX corpus example."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import shutil
import sys
import tempfile
import zipfile
from pathlib import Path
from typing import Any


HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(HERE.parent / "pptx-jxr-profiler"))

import corpus
import profiler


CASE_PPTX = Path("FunctionalTests/Text/SLIDESNET-44192/SLIDESNET-44192.pptx")
CASE_ENTRY = "ppt/media/hdphoto1.wdp"
CASE_SHA256 = "4dadf2d592d85782a3f6c0a4de8aa9edd130921064954d6bfe0c2220550500e5"
MAX_ENTRY_BYTES = 512 * 1024 * 1024
MAX_MAE = 8.0
MAX_COMPONENT_DELTA = 96


def read_manifest_profile(manifest: Path) -> dict[str, Any]:
    with manifest.open("r", encoding="utf-8") as stream:
        for line in stream:
            if not line.strip():
                continue
            row = json.loads(line)
            if (row.get("sha256") == CASE_SHA256 and
                    row.get("pptx") == CASE_PPTX.as_posix() and
                    row.get("entry") == CASE_ENTRY):
                return row
    raise ValueError("the fixed sample is missing from the committed corpus manifest")


def assert_expected_profile(profile: dict[str, Any]) -> None:
    expected = {
        "width": 174,
        "height": 87,
        "source_bit_depth": "8bpp",
        "source_color_format": "CF_RGB",
        "pixel_format": "Bgr24",
        "plane_color_format": "YUV_444",
        "bitstream_layout": "frequency",
        "alpha_mode": "none",
        "overlap": 0,
        "tile_columns": 1,
        "tile_rows": 1,
        "codestream_version": 1,
        "codestream_subversion": 1,
        "subbands": "all",
    }
    differences = ["%s expected=%r actual=%r" % (name, value,
        profile.get(name)) for name, value in expected.items()
        if profile.get(name) != value]
    if differences:
        raise ValueError("sample profile changed: " + "; ".join(differences))
    quantizers = profile.get("frame_quantizers") or {}
    expected_qps = {"dc": [16, 27, 27], "lp": [16, 36, 36],
                    "hp": [24, 42, 42]}
    for band, values in expected_qps.items():
        actual = (quantizers.get(band) or {}).get("stored_indices")
        if actual != values:
            raise ValueError("sample %s quantizer changed: %r" % (band, actual))


def read_package_jxr_names(package: zipfile.ZipFile) -> list[str]:
    names = []
    for info in package.infolist():
        suffix = Path(info.filename).suffix.lower()
        with package.open(info, "r") as stream:
            prefix = stream.read(16)
        if suffix not in profiler.JXR_EXTENSIONS and not profiler.is_jxr_signature(prefix):
            continue
        if info.file_size > MAX_ENTRY_BYTES:
            raise ValueError("candidate image exceeds the configured size limit")
        data = package.read(info)
        if not profiler.is_jxr_signature(data[:16]):
            if suffix not in profiler.JXR_EXTENSIONS:
                continue
            raise ValueError("JXR-like package entry has no JPEG XR signature: " +
                             info.filename)
        profiler.parse_jxr(data)
        names.append(info.filename)
    return names


def replace_zip_entry(source_path: Path, output_path: Path,
                      entry_name: str, replacement: bytes) -> dict[str, Any]:
    """Copy a ZIP package, replacing one member and verifying all other payloads."""
    source_resolved = source_path.resolve()
    output_resolved = output_path.resolve()
    if source_resolved == output_resolved:
        raise ValueError("output PPTX must not overwrite its source")
    output_path.parent.mkdir(parents=True, exist_ok=True)
    if output_path.exists():
        raise FileExistsError("refusing to overwrite existing output: " + str(output_path))
    partial = output_path.with_name(output_path.name + ".partial")
    if partial.exists():
        raise FileExistsError("temporary output already exists: " + str(partial))

    source_hashes: dict[str, str] = {}
    source_names: list[str] = []
    source_comment = b""
    try:
        with zipfile.ZipFile(source_path, "r") as source:
            if source.testzip() is not None:
                raise ValueError("source PPTX has a corrupt ZIP member")
            infos = source.infolist()
            source_names = [info.filename for info in infos]
            if len(source_names) != len(set(source_names)):
                raise ValueError("source PPTX contains duplicate ZIP member names")
            if entry_name not in source_names:
                raise ValueError("target JXR entry is missing from source PPTX")
            source_comment = source.comment
            with zipfile.ZipFile(partial, "w") as output:
                output.comment = source_comment
                for info in infos:
                    payload = replacement if info.filename == entry_name else source.read(info)
                    source_hashes[info.filename] = hashlib.sha256(
                        source.read(info) if info.filename != entry_name else payload
                    ).hexdigest()
                    output.writestr(info, payload)

        with zipfile.ZipFile(partial, "r") as check:
            if check.testzip() is not None:
                raise ValueError("replacement PPTX has a corrupt ZIP member")
            check_names = [info.filename for info in check.infolist()]
            if check_names != source_names or check.comment != source_comment:
                raise ValueError("replacement PPTX changed package members or comment")
            changed: list[str] = []
            for name in source_names:
                actual = hashlib.sha256(check.read(name)).hexdigest()
                if actual != source_hashes[name]:
                    changed.append(name)
            unexpected = [name for name in changed if name != entry_name]
            if unexpected:
                raise ValueError("unexpected ZIP payload changes: " +
                                 ", ".join(unexpected))
            if hashlib.sha256(check.read(entry_name)).digest() != hashlib.sha256(
                    replacement).digest():
                raise ValueError("replacement JXR payload differs inside PPTX")
        os.replace(str(partial), str(output_path))
        return {"zip_entries": len(source_names),
                "unchanged_entries": len(source_names) - 1,
                "target_entry": entry_name,
                "target_sha256": hashlib.sha256(replacement).hexdigest()}
    except Exception:
        if partial.exists():
            partial.unlink()
        raise


def encode_profile(runner: Path, source_path: Path, pixels_path: Path,
                   output_path: Path) -> None:
    command = [str(runner), "encode-profile", str(source_path), "bgr",
               str(pixels_path), str(output_path)]
    code, output = corpus.run_process(command, runner.parent)
    if code != 0 or not output_path.is_file():
        raise RuntimeError("profile encoder failed (%d): %s" % (code, output))


def channel_metrics(expected: bytes, actual: bytes, channels: int
                    ) -> dict[str, Any]:
    if len(expected) != len(actual):
        raise ValueError("round-trip pixel buffers have different lengths")
    metrics = corpus.difference_metrics(expected, actual, channels)
    metrics["mean_absolute_error"] = (
        sum(abs(left - right) for left, right in zip(expected, actual)) /
        float(len(expected)) if expected else 0.0)
    return metrics


def powershell_quote(value: str) -> str:
    return "'" + value.replace("'", "''") + "'"


def validate_with_aspose(assembly: Path, source_pptx: Path,
                         output_pptx: Path) -> str:
    powershell = shutil.which("pwsh")
    if not powershell:
        raise RuntimeError("PowerShell 7 (pwsh) is required for target-project validation")
    if not assembly.is_file():
        raise FileNotFoundError("Aspose.Slides assembly not found: " + str(assembly))
    script = (
        "$ErrorActionPreference='Stop'; "
        "Add-Type -Path " + powershell_quote(str(assembly)) + "; "
        "$src=[Aspose.Slides.Presentation]::new(" +
        powershell_quote(str(source_pptx)) + "); "
        "$dst=[Aspose.Slides.Presentation]::new(" +
        powershell_quote(str(output_pptx)) + "); "
        "try { "
        "if ($src.Slides.Count -ne $dst.Slides.Count -or "
        "$src.Images.Count -ne $dst.Images.Count) { "
        "throw 'Aspose.Slides package content counts changed'; } "
        "'slides=' + $dst.Slides.Count + ' images=' + $dst.Images.Count "
        "} finally { $src.Dispose(); $dst.Dispose(); }"
    )
    code, output = corpus.run_process([powershell, "-NoProfile", "-Command",
                                       script], assembly.parent)
    if code != 0 or "slides=" not in output or "images=" not in output:
        raise RuntimeError("Aspose.Slides could not validate the output PPTX: " + output)
    return output


def run_case(root: Path, runner: Path, native_decoder: Path,
             output_pptx: Path, report_path: Path,
             manifest_path: Path, aspose_assembly: Path) -> dict[str, Any]:
    row = read_manifest_profile(manifest_path)
    profile = row["profile"]
    assert_expected_profile(profile)
    source_pptx = (root / CASE_PPTX).resolve()
    if not source_pptx.is_relative_to(root.resolve()) or not source_pptx.is_file():
        raise FileNotFoundError("the fixed PPTX sample is missing under the corpus root")

    with zipfile.ZipFile(source_pptx, "r") as package:
        if package.testzip() is not None:
            raise ValueError("source sample PPTX failed ZIP integrity check")
        jxr_names = read_package_jxr_names(package)
        if jxr_names != [CASE_ENTRY]:
            raise ValueError("fixed sample no longer contains exactly its one JXR entry: %r" %
                             jxr_names)
        source_jxr = package.read(CASE_ENTRY)
    if hashlib.sha256(source_jxr).hexdigest() != CASE_SHA256:
        raise ValueError("fixed sample JXR SHA-256 differs from the pinned corpus manifest")

    report: dict[str, Any] = {
        "schema_version": 1,
        "case": "SLIDESNET-44192-single-frequency-no-alpha",
        "source_pptx": CASE_PPTX.as_posix(),
        "source_entry": CASE_ENTRY,
        "source_jxr_sha256": CASE_SHA256,
        "source_profile": profile,
        "status": "running",
    }
    with tempfile.TemporaryDirectory(prefix="jxr-step9-") as temp_dir:
        work = Path(temp_dir)
        source_jxr_path = work / "source.jxr"
        encoded_jxr_path = work / "roundtrip.jxr"
        source_jxr_path.write_bytes(source_jxr)

        source_managed_profile = corpus.managed_source_profile(
            runner, source_jxr_path, work)
        if not source_managed_profile.get("packet_syntax_complete"):
            raise ValueError("managed source profile packet parse is incomplete")
        source_profile_differences = corpus.compare_managed_profile(
            profile, source_managed_profile)
        if source_profile_differences:
            raise ValueError("managed source profile differs from pinned profile: " +
                             "; ".join(source_profile_differences))

        source_pixels = corpus.managed_pixels(runner, source_jxr_path,
                                              work, "color")
        source_width, source_height, source_channels, source_data = source_pixels
        if (source_width, source_height, source_channels) != (174, 87, 3):
            raise ValueError("managed decoder returned unexpected source image shape")
        source_native_pixels = corpus.native_pixels(native_decoder,
            source_jxr_path, work, "color")
        if source_native_pixels != source_pixels:
            source_metrics = channel_metrics(source_native_pixels[3], source_data, 3)
            raise ValueError("chosen source is not pixel-identical between managed and C "
                             "decoders: %r" % source_metrics)

        pixels_path = work / "source-pixels.bin"
        pixels_path.write_bytes(source_data)
        encode_profile(runner, source_jxr_path, pixels_path, encoded_jxr_path)
        encoded = encoded_jxr_path.read_bytes()
        output_profile = corpus.managed_source_profile(runner,
            encoded_jxr_path, work)
        output_profile_differences = corpus.compare_managed_profile(
            profile, output_profile, compare_serialized_lengths=False)
        if output_profile_differences:
            raise ValueError("re-encoded JXR changed the semantic profile: " +
                             "; ".join(output_profile_differences))
        if not output_profile.get("packet_syntax_complete"):
            raise ValueError("re-encoded JXR packet parse is incomplete")

        managed_output = corpus.managed_pixels(runner, encoded_jxr_path,
                                               work, "color")
        native_output = corpus.native_pixels(native_decoder, encoded_jxr_path,
                                             work, "color")
        if managed_output[:3] != source_pixels[:3] or native_output[:3] != source_pixels[:3]:
            raise ValueError("a decoder returned dimensions or channels different from source")
        managed_metrics = channel_metrics(source_data, managed_output[3], 3)
        native_metrics = channel_metrics(source_data, native_output[3], 3)
        if (managed_metrics["mean_absolute_error"] > MAX_MAE or
                managed_metrics["maximum_component_delta"] > MAX_COMPONENT_DELTA or
                native_metrics["mean_absolute_error"] > MAX_MAE or
                native_metrics["maximum_component_delta"] > MAX_COMPONENT_DELTA):
            raise ValueError("re-encoded JXR exceeded the pixel quality gate")

        package_validation = replace_zip_entry(source_pptx, output_pptx,
                                                CASE_ENTRY, encoded)
        with zipfile.ZipFile(output_pptx, "r") as package:
            embedded = package.read(CASE_ENTRY)
            if hashlib.sha256(embedded).hexdigest() != package_validation["target_sha256"]:
                raise ValueError("JXR extracted from output PPTX differs from encoded output")
        target_validation = validate_with_aspose(aspose_assembly,
                                                 source_pptx, output_pptx)
        report.update({
            "output_pptx": str(output_pptx.resolve()),
            "output_jxr_sha256": hashlib.sha256(encoded).hexdigest(),
            "output_profile": output_profile,
            "source_managed_equals_native": True,
            "managed_roundtrip_quality": managed_metrics,
            "native_roundtrip_quality": native_metrics,
            "package_validation": package_validation,
            "target_project_validation": target_validation,
            "status": "passed",
        })

    report_path.parent.mkdir(parents=True, exist_ok=True)
    report_path.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n",
                           encoding="utf-8")
    return report


def make_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", required=True, type=Path,
                        help="PPTX corpus root")
    parser.add_argument("--managed-runner", required=True, type=Path,
                        help="built .NET Framework 2.0 corpus bridge")
    parser.add_argument("--native-decoder", required=True, type=Path,
                        help="built C reference decoder")
    parser.add_argument("--aspose-slides-assembly", required=True, type=Path,
                        help="target project's built Aspose.Slides.dll")
    parser.add_argument("--output-pptx", required=True, type=Path,
                        help="new output path; existing files are never overwritten")
    parser.add_argument("--report", required=True, type=Path,
                        help="JSON result path")
    parser.add_argument("--manifest", type=Path,
                        default=HERE / "corpus-manifest.jsonl",
                        help="pinned corpus manifest")
    return parser


def main(argv: list[str] | None = None) -> int:
    args = make_parser().parse_args(argv)
    try:
        report = run_case(args.root.resolve(), args.managed_runner.resolve(),
            args.native_decoder.resolve(), args.output_pptx.resolve(),
            args.report.resolve(), args.manifest.resolve(),
            args.aspose_slides_assembly.resolve())
        print("Step 9 single-case round trip passed")
        print("Output PPTX: %s" % report["output_pptx"])
        print("Output JXR SHA-256: %s" % report["output_jxr_sha256"])
        return 0
    except Exception as error:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        failure = {
            "schema_version": 1,
            "case": "SLIDESNET-44192-single-frequency-no-alpha",
            "source_pptx": CASE_PPTX.as_posix(),
            "source_entry": CASE_ENTRY,
            "source_jxr_sha256": CASE_SHA256,
            "output_pptx": str(args.output_pptx.resolve()),
            "status": "failed",
            "error": str(error),
        }
        args.report.resolve().write_text(json.dumps(failure, ensure_ascii=False,
            indent=2) + "\n", encoding="utf-8")
        print("Step 9 single-case round trip failed: %s" % error,
              file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
