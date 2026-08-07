# Minimal JPEG XR profile fixture

This fixture fixes the smallest complete bitmap-to-JPEG-XR-to-bitmap route implemented by the project:

- 16x16 pixels: exactly one JPEG XR macroblock, with no boundary padding;
- 8-bit grayscale indexed BMP, with a canonical 256-entry grayscale palette and 3779 px/m (96 DPI) in both axes;
- `Y_ONLY`, so no RGB/YCoCg conversion and no chroma planes;
- QP 1 (`-q 1`): lossless coding;
- no overlap (`-l 0`), alpha, tiles, flexbit trimming, skipped subbands, ROI, thumbnail, rotation or post-processing;
- spatial bitstream (`-f`) and one tile: no frequency substreams or index table.

Run `create-minimal-gray-bmp.ps1` to regenerate the source BMP, then run `convert-minimal-profile.cmd` after building the x64 Release solution. The batch uses the following fixed commands:

```bat
JXREncApp.exe -i minimal-gray-16x16.bmp -o minimal-gray-16x16.jxr -c 2 -d 0 -q 1 -l 0 -f
JXRDecApp.exe -i minimal-gray-16x16.jxr -o minimal-gray-16x16-restored.bmp -c 2 -a 0 -p 0
```

Run `trace-minimal-profile.cmd` to produce JSON snapshots in `minimal-profile/trace`. The new `-X <directory>` option is accepted by both command-line utilities.

For the minimal profile, the trace contains encoder snapshots for centered samples, transformed coefficients, quantized coefficients and predicted coefficients; decoder snapshots after DC, LP, DC/LP prediction, dequantization, HP, AC prediction and inverse transform. Each utility also emits a `*-global.json` with normalized codec settings.

The trace now records the entropy-coding boundary as well. `encoder-mb-*-bitstream-{dc,lp,hp}.json` and their decoder counterparts describe the exact half-open bit ranges consumed by the DC, LP and HP portions of each macroblock. `encoder-bitstream.jxr` and `decoder-bitstream.jxr` are binary copies of the full JXR input/output stream; the ranges index those copies. In the minimal profile, the encoder and decoder ranges must match exactly.

## Algorithmic path

### BMP to JXR

1. The BMP decoder reads the 16x16, 8-bit indexed image and recognizes its canonical palette as `8bppGray`.
2. The format converter passes the same grayscale samples to the JPEG XR encoder; there is no RGB/BGR conversion, alpha handling, bit-depth conversion or chroma plane.
3. The encoder writes the JXR container and creates one `Y_ONLY`, spatial-layout image stream with one tile and no index table.
4. `inputMBRow` loads the 16x16 image as the single macroblock and recenters samples around zero. Because both dimensions are multiples of 16, no image-boundary padding is needed.
5. The codec performs its required forward 4x4/macroblock transforms, DC/LP/HP coefficient partitioning, prediction, run/level representation and adaptive Huffman coding.
6. QP 1 uses the codec's lossless quantization path; `OL_NONE` skips overlap; no subsampling, flexbit trimming, skipped subbands or alpha stream is present.

### JXR to BMP

1. The decoder reads the JXR container and `WMPHOTO` header, restoring the `Y_ONLY`, 8-bit, one-tile, spatial, no-overlap parameters.
2. It entropy-decodes DC/LP/HP data, reverses run/level and coefficient prediction, performs lossless dequantization, then inverse 4x4/macroblock transforms.
3. No overlap, chroma interpolation, color conversion, alpha merge, ROI, thumbnail, orientation transform or post-processing runs.
4. The Y samples are recentered to 8-bit grayscale values and the BMP writer emits the canonical palette and the original 3779 px/m resolution.

The source and restored BMP files are byte-for-byte identical, so this fixture validates the complete minimal JPEG XR round trip rather than pixel equivalence alone.
