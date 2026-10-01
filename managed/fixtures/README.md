# Gray no-overlap reference images

`create-gray-sizes.ps1` deterministically generates Gray8 BMP files, encodes
them with the native Release x64 application using `-c 2 -d 0 -q 1 -l 0 -f`,
and decodes each JXR back to a BMP. The script verifies that each native
round-trip restores the BMP byte-for-byte.

The fixtures cover 1×1, 15×17, 16×16, 32×32, 31×19, 17×1, 33×18, and
257×17. The last case crosses the native 16-macroblock scan-reset boundary.
`Jxr.Managed.Tests` independently decodes each native JXR to pixels and
requires the managed encoder to reproduce it byte-for-byte. It also checks
the Gray BMP adapter and managed round-trip. `*-restored.bmp` files are the
native decoder outputs, retained as additional reference artifacts.

`create-gray-quality.ps1` generates lossy/profile fixtures from
`gray-31x19.bmp` with the native Release x64 encoder. `q16-all`, `q64-all`,
`q1-no-flex`, `q2-all`, `q255-all`, `q16-no-flex`, `q16-trim3`,
`q16-trim15`, `q16-no-hp`, and `q16-dc-only` retain the source
JXR, native-decoded BMP, and `-X` structured trace. The managed conformance
test compares complete JXR bytes and, per macroblock, native quantized and
predicted coefficient arrays plus exact DC/LP/HP bit boundaries. It also
compares managed-decoded pixels to the native decoder output for every mode.
The native transform snapshot occurs before the delayed macroblock transform
finishes and therefore is not directly equivalent to the managed
pre-quantization snapshot.

`create-rgb444-quality.ps1` uses the two existing real RGB24 BMP images to
generate spatial YUV 4:4:4, no-overlap JPEG XR streams with native encoder
options `-c 0 -d 3 -q 16 -l 0 -f -p`. The sign image covers all subbands,
no flexbits, no highpass, DC-only and trim-flexbits=3; the city image covers
all subbands. Each JXR and native-restored BMP is retained, and
`rgb444_quality_native_fixtures` compares every RGB pixel byte after managed
decoding. These fixtures exercise lossy scaled arithmetic, including the
chroma normalization between inverse-transform stages.

`create-rgb444-small.ps1` creates a deterministic 15x17 RGB24 BMP, native
lossless YUV 4:4:4 JXR and native-restored BMP. The restored BMP equals the
source byte-for-byte. `rgb444_sizes_round_trip` compares the entire managed
JXR with this native reference, then decodes it to the original pixels. The
real sign and city images above also serve as native byte-for-byte encoder
references, including the QP=16 subband and flexbit variants.

`create-overlap-subsampled.ps1` generates deterministic 32×32, 31×19 and
1×1 RGB24 BMPs, plus Gray overlap references. It uses the native spatial,
sequential encoder to produce YUV 4:4:4 overlap and YUV 4:2:2/4:2:0 streams
at OL_NONE, OL_ONE and OL_TWO. The QP=16 fixtures exercise scaled arithmetic;
the QP=1 fixtures pass `-u` and exercise unscaled arithmetic. Native-restored
BMPs are retained for pixel comparison. The managed conformance tests require
byte-identical JXR and native-identical decoded pixels. The native encoder
rejects OL_TWO for one-macroblock-wide subsampled input, so the 1×1 fixtures
cover OL_ONE and the public API tests the corresponding rejection.

`create-spatial-tiles.ps1` generates C-reference spatial streams with an index
table for tiled decoding. The set includes Gray 32×32, RGB 4:4:4/4:2:2/4:2:0
32×32 uniform 2×2 grids at OL_NONE and OL_TWO, a 48×32 4:2:2 image with
variable tile widths of one and two macroblocks, and a 31×19 4:2:0 edge case
at QP 1/OL_TWO. Each JXR is paired with the native decoder's restored BMP.
`spatial_tile_native_fixtures` compares managed output pixels with those BMPs;
`spatial_tile_encode_native_fixtures` requires the managed encoder to reproduce
all ten JXR files byte-for-byte and `spatial_tile_invalid_tables` verifies
rejection of malformed index markers, out-of-range packet offsets and
incorrect tile IDs. The native CLI fixtures use soft tile boundaries;
hard-boundary encoding/decoding does not yet have a native reference fixture.

`create-frequency-layout.ps1` generates native progressive frequency-layout
references for Gray, RGB/YUV 4:4:4, 4:2:2 and 4:2:0. It also creates a
sequential 2×2 tiled Gray stream and QP16 references for all subbands, including
DC-only, no-highpass, skipped flexbits and trimmed flexbits. Each stream is paired with the native decoder's
restored BMP. `frequency_layout_native_fixtures` checks byte-identical JXR
encoding and compares managed decoded pixels with those native BMPs.

`create-alpha.ps1` creates a deterministic 32×32 BGRA image and C-reference
planar-alpha streams at color QP 16 and alpha QP 1/16 (`-a 2 -Q`). The managed
`planar_alpha_native_fixtures` test requires byte-identical JXR output, checks
the container's absolute `AlphaOffset`/`AlphaByteCount`, compares alpha-only,
color-only and BGRA decode results to native BMPs, and rejects truncated or
out-of-range alpha streams. It also exercises a managed frequency-layout
stream with a 2×2 tile grid. Interleaved alpha (`-a 3`) is not implemented and
is reserved for a future step: its second plane shares the primary packet
pipeline and cannot be represented as the separate planar codestream used here.
