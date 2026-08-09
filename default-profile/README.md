# Default encoder and decoder fixture

`city-park-605x478.bmp` is a 24bpp BGR BMP prepared from the supplied photographic image. The non-aligned 605×478 dimensions exercise real image content and edge macroblocks.

This fixture deliberately uses the current utility defaults. `-c 0` is the only codec option because the utilities require the input/output pixel format; no quality, subsampling, overlap, layout, progression, alpha, tile or trace option is supplied.

```text
JXREncApp.exe -i city-park-605x478.bmp -o city-park-605x478.jxr -c 0
JXRDecApp.exe -i city-park-605x478.jxr -o city-park-605x478-restored.bmp -c 0
```

The source BMP has canonical headers, so the restored BMP is byte-identical. `JxrConformanceTests default_image_round_trip` regenerates the JXR and BMP in `tests/work/`, then compares them with all three tracked fixtures. This test protects the established non-minimal encoder/decoder behavior during the managed-port refactor.
