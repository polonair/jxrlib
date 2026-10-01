# JPEG XR compatibility baseline

This is the baseline for replacing the C command-line application with a
fully managed C# library targeting .NET Framework 2.0. It describes what is
**integrated and tested today**, not everything for which a C# algorithmic
helper already exists. Update this matrix whenever an end-to-end mode becomes
supported; keep the C reference and its fixtures as the compatibility oracle.

## Status vocabulary

- **Reference fixture**: the C utilities regenerate the tracked JXR and BMP
  byte-for-byte in `JxrConformanceTests`. This is stronger than an option
  merely appearing in the C utility's help text.
- **Managed end-to-end**: a public managed entry point accepts the specified
  input, produces the specified output, and has a fixture-backed test.
- **Managed component**: one or more internal stages are ported and tested,
  but no complete file conversion is available for that mode.
- **Not integrated**: no managed end-to-end claim. This does not assert that
  the C reference cannot perform the operation.

## End-to-end fixtures and comparison contract

| Case | C command-line profile | Reference check | Managed result |
| --- | --- | --- | --- |
| Minimal Gray, 16×16, one macroblock | 8bpp Gray; `-c 2 -d 0 -q 1 -l 0 -f`; decoder `-c 2 -a 0 -p 0` | `minimal_round_trip`: JXR and restored BMP byte-identical; `bit_ranges`/`entropy_trace` check DC/LP/HP trace | **End-to-end**: `JxrMinimalEncoder.EncodeGrayBmp` and `JxrMinimalDecoder.DecodeGrayBmp`; exact fixture JXR/BMP, plus 18 generated Gray patterns |
| Real RGB sign, 334×330 | 24bpp BGR; `-c 0 -d 3 -q 1 -l 0 -f -p`; decoder `-c 0 -a 0 -p 0` | `real_image_round_trip`: JXR and restored BMP byte-identical; exercises edge macroblocks | **Component only**: color pixel pipeline, headers and entropy vectors; no full managed conversion |
| Default RGB photograph, 605×478 | 24bpp BGR; only `-c 0` on each utility, thus native quality/layout/overlap/progression defaults | `default_image_round_trip`: JXR and restored BMP byte-identical; exercises non-aligned size and default behavior | **Component only**: pixel pipeline, header parser and codec primitives; no full managed conversion |
| Spatial Gray/RGB, 2×2 tiles and index table | Native spatial profiles; Gray QP 1/16, RGB 4:4:4/4:2:2/4:2:0 QP 16; OL_NONE/OL_TWO | `spatial_tile_native_fixtures`: decoded pixels compared with native-restored BMP; `spatial_tile_encode_native_fixtures`: JXR bytes compared with C encoder | **Managed end-to-end encode/decode**: tracked fixtures cover soft tiles and indexed spatial packets |
| Spatial variable-width/edge tiles | RGB 4:2:2 48×32 with 1+2 tile columns; RGB 4:2:0 31×19, 2×2, QP 1, OL_TWO | Byte-identical JXR output and native-restored pixel comparison; validates nonuniform boundaries and partial edge blocks | **Managed end-to-end encode/decode**; invalid layouts, index tables and packet tile IDs are rejected |

The exact commands and tracked input/output files are in
[`minimal-profile/README.md`](../minimal-profile/README.md),
[`real-image-profile/README.md`](../real-image-profile/README.md) and
[`default-profile/README.md`](../default-profile/README.md). The C round-trip tests write to `tests/work/`,
not over the tracked fixtures. The managed test
`headers_reference_fixtures` parses all three JXR files, and
`image_pipeline_bitmap_fixtures` checks pixel conversion on all three BMPs;
neither test is a color JXR decoder or encoder.

## Capability matrix

The C columns below distinguish an advertised/implemented option from a
fixture-backed one. The option inventory comes from `JxrEncApp.c` and
`JxrDecApp.c`; enumerating an option is **not** a claim that every combination
has been conformance-tested.

| Capability | C reference | Managed library now | Evidence / remaining boundary |
| --- | --- | --- | --- |
| 8bpp Gray, 16×16, Y_ONLY, lossless, spatial, no overlap, one tile | Reference fixture | **End-to-end** encode/decode | Minimal fixture and managed `minimal_*_end_to_end` tests |
| Gray at other dimensions / multiple macroblocks / partial edge blocks | Native Gray fixtures | **Managed end-to-end** | Gray 1×1 through 257×17; managed encoder byte-matches native fixtures and decoder matches native restored pixels |
| 24bpp BGR/RGB 4:4:4, spatial | Reference and tiled fixtures | **Managed end-to-end** | Encoder and decoder cover one tile and indexed 2×2 soft-boundary tiles |
| Native default RGB encode/decode, including frequency layout and overlap | Reference fixture | Partially integrated | Frequency packets are covered for current 8-bit formats, but the full default city profile and frequency+overlap combinations are not yet fixture-backed |
| Gray quantization/quality, skipped subbands and flexbit trimming in frequency layout | C encoder options `-q`, `-s`, `-F` | **Managed end-to-end** for fixture profiles | QP16 all-subband, no-highpass, DC-only, no-flexbit and trim=2 Gray streams match C bytes and native-decoded pixels |
| YUV 4:2:0 / 4:2:2, 8-bit RGB output | C pixel-format and `-d` options | **Managed end-to-end decode** | Native fixtures cover 4:2:2/4:2:0 and spatial tiles; other depths and CMYK/RGBE remain unintegrated |
| Overlap levels 0/1/2, spatial progression | C options `-l`, `-p`, `-f` | **Managed end-to-end decode** for Gray/RGB spatial profiles | Native fixture-backed coverage includes tiled OL_NONE/OL_TWO and earlier untiled OL_NONE/ONE/TWO; frequency+overlap still needs fixtures |
| Spatial tiles and index table | C options `-U`, `-V`, `-H` | **Managed end-to-end encode/decode** | Gray/RGB fixtures exercise uniform and variable soft-boundary tile layouts. Hard-boundary encoding/decoding has no reference fixture yet. |
| Frequency packets (DC/LP/HP/flexbits), progressive and sequential order | C default and `-p` | **Managed end-to-end encode/decode** for current 8-bit Gray/RGB profiles | `frequency_layout_native_fixtures` checks byte-identical C streams for Gray, RGB/YUV 4:4:4/4:2:2/4:2:0, QP16 trim/skip, progressive and sequential 2×2 tiles |
| Alpha channel / planar or interleaved alpha | C `-a` and `-Q` | Component only | Session and image-plane helpers; no managed alpha file conversion |
| Decode ROI, thumbnail, orientation, post-processing | C decoder `-r`, `-T`, `-O`, `-p` | Component only | Transcoder ROI/orientation and other stage vectors do not constitute a decoder entry point |
| JXR-to-JXR compressed-domain transcode | C decoder output `.jxr` and `-s` | Component only | `JxrTranscoder` handles coefficient/ROI operations, not file-to-file packet decode/write |
| BMP/TIFF/HDR input, BMP/TIFF output | C application file adapters (format-dependent) | Canonical 8bpp BMP adapter only | `JxrCodec` now accepts/returns pixel buffers and JPEG XR streams; no general BMP, TIFF or HDR adapter yet |
| Container and codestream headers | C reader/writer | Read component; field-based minimal Gray writer | `JxrHeaders.Read` parses all three fixtures; managed writer emits minimal container/header/packet fields and computes codestream length |
| Native CLI parity | `JXREncApp`/`JXRDecApp` | Not integrated | No managed command-line replacement yet |

The advertised C format lists include some commented-out or explicitly
rejected entries. Those are **not** counted as supported C modes here. In
particular, this matrix must not be interpreted as a claim of complete JPEG XR
standard coverage by either implementation.

## Regression gate for subsequent tasks

1. Build `jxrencoderdecoder/JXR_vc14.sln` in `Release|x64`; run the full native
   `JxrConformanceTests` suite, including the three fixture round trips.
2. Build `managed/Jxr.Managed.sln` using Framework v2.0 MSBuild; run all
   `Jxr.Managed.Tests` tests. The minimal JXR and BMP must remain byte-identical
   to the C fixtures.
3. For each newly integrated row, add a **managed end-to-end** test against a
   tracked C fixture (or freeze a new fixture with its exact CLI arguments).
   Do not promote a row from component to end-to-end based on isolated vectors.
4. Check `git diff --check` and commit only after the applicable gates pass.

The next integration boundary is more complex packet/index structures,
frequency-plus-overlap combinations, and general session flushing; the current
frequency tests cover the frozen 8-bit profiles listed above.
