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
