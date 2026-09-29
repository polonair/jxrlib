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
`q16-no-flex`, `q16-trim3`, `q16-no-hp`, and `q16-dc-only` retain the source
JXR, native-decoded BMP, and `-X` structured trace. The managed conformance
test compares complete JXR bytes and, per macroblock, native quantized and
predicted coefficient arrays plus available DC/LP/HP bit counts. It also
compares managed-decoded pixels for every mode except no-flexbits, whose
managed HP decoder still has a known end-of-stream defect; its encoded JXR
and coefficient/bit-range trace are verified exactly.
