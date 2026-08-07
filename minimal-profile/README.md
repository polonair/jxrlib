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
