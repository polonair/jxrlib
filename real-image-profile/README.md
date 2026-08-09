# Real-image lossless fixture

`test-sign-334x330.bmp` is a 24bpp BGR BMP prepared from the supplied real image. Its deliberately non-aligned 334×330 dimensions exercise edge macroblocks as well as the RGB colour path.

The fixed encoder profile is lossless RGB 4:4:4, spatial and sequential, with no overlap:

```text
JXREncApp.exe -i test-sign-334x330.bmp -o test-sign-334x330.jxr -c 0 -d 3 -q 1 -l 0 -f -p
JXRDecApp.exe -i test-sign-334x330.jxr -o test-sign-334x330-restored.bmp -c 0 -a 0 -p 0
```

The source BMP has canonical headers, so the restored BMP is byte-identical. Both BMP files and the JXR are tracked fixtures. `JxrConformanceTests real_image_round_trip` regenerates the JXR and BMP in `tests/work/`, then compares them to those fixtures.
