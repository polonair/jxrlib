# Managed JPEG XR port

This directory contains the fully managed C# port, kept separate from the
native reference implementation.  Every project targets .NET Framework 2.0
and uses no P/Invoke, `unsafe` code or external test framework.

`Jxr.Managed.Core` now includes bit I/O, adaptive entropy state, DC/LP/HP
decoding and coefficient storage. The `JxrQuantization` module ports native
`remapQP`, channel-mode expansion, signed integer coefficient quantization,
DC/LP/HP macroblock placement and decoder DC/LP dequantization. The quantizer
is represented by integer QP, offset, unsigned reciprocal mantissa and
exponent; the coding path never uses floating-point approximations.

The native and managed tests `quantization_reference_vectors` compare a
signature covering all 256 QP indices, scaled/unscaled arithmetic, both
chroma shifts, negative/positive coefficients, DC rounding and inverse
multiplication. `quantization_channel_modes` checks modes 0–3.
`quantization_macroblock_vectors` compares all coefficient and DC/LP buffer
positions for Y-only, YUV 4:4:4, 4:2:2 and 4:2:0, with all subbands, no HP,
DC-only and transcode bypass. The minimal JXR fixture also checks managed
dequantization against the decoder's 256-coefficient native trace.

Build and run on this machine:

```powershell
& 'C:\Windows\Microsoft.NET\Framework\v2.0.50727\MSBuild.exe' .\Jxr.Managed.sln /p:Configuration=Release
& .\Jxr.Managed.Tests\bin\Release\Jxr.Managed.Tests.exe
```

The native `JxrConformanceTests` remains the reference oracle and must stay
green alongside the managed runner.

`JxrImagePipeline` currently covers the full-resolution 8-bit pixel boundary:
Gray/RGB input centering, reversible RGB/CMYK color transforms, scaled and
unscaled RGB/Gray output, clipping, and explicit row strides. Its native and
managed reference vectors share a frozen signature. It does not yet compose
headers, entropy coding, transforms, tiles, alpha, or BMP container I/O into a
complete image encoder/decoder.

`JxrDecoderSession` and `JxrEncoderSession` now own separate two-row
coefficient buffers for each channel and optional alpha plane. Their memory
plans reproduce the native decoder/encoder layout formulas, including the
32-bit safety decisions, while actual managed allocations are `int[]` rather
than a native struct-and-pointer slab. Sessions can be configured from parsed
JXR headers and release their buffers through `Dispose`. Session execution,
packet I/O, and final encoder flushing are not wired up yet.
