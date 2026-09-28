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

`JxrMinimalDecoder.DecodeGrayBmp` now connects the ported modules into a
complete, fully managed decoder for the frozen 16x16 8-bit Y-only fixture.
It accepts a JXR container or raw codestream, locates the single spatial
packet from the header, decodes DC/LP/HP, applies prediction, dequantization
and inverse transform, and emits an 8-bit grayscale BMP. The
`minimal_decoder_end_to_end` test compares the entire BMP byte-for-byte with
both the source and native-decoded fixture. Unsupported JPEG XR profiles
(including color, larger images, overlap and tiles) are explicitly rejected;
this is not yet a general-purpose JPEG XR decoder.

`JxrMinimalEncoder.EncodeGrayBmp` now connects the managed forward transform,
quantization, coefficient prediction, adaptive entropy state and bit writer.
It accepts a canonical 16x16 8-bit grayscale BMP (including the grayscale
palette and 3779 px/m resolution), and writes the corresponding lossless
Y-only JXR. The fixed profile's TIFF/JPEG XR header is serialized from a
constant header template; the codestream length is updated for each image,
while all coefficient and entropy data are computed from the input pixels.
The fixture JXR is reproduced byte-for-byte, and eighteen additional sample
patterns round-trip through the managed encoder and decoder. Arbitrary image
sizes, color, tiles and other JPEG XR profiles are still unsupported.

`JxrImagePipeline` currently covers the full-resolution 8-bit pixel boundary:
Gray/RGB input centering, reversible RGB/CMYK color transforms, scaled and
unscaled RGB/Gray output, clipping, and explicit row strides. Its native and
managed reference vectors share a frozen signature. General-purpose composition
with tiles, alpha and arbitrary image sizes remains future work.

`JxrDecoderSession` and `JxrEncoderSession` now own separate two-row
coefficient buffers for each channel and optional alpha plane. Their memory
plans reproduce the native decoder/encoder layout formulas, including the
32-bit safety decisions, while actual managed allocations are `int[]` rather
than a native struct-and-pointer slab. Sessions can be configured from parsed
JXR headers and release their buffers through `Dispose`. The minimal decoder
now executes one session and packet; general session execution and final
encoder flushing are not wired up yet.

`JxrTranscoder` ports the coefficient-domain core: all eight orientations,
DC/AC sign and position changes for 4:4:4, 4:2:2 and 4:2:0, and ROI expansion
to macroblock boundaries with overlap. The native and managed vector suites
compare both mutated source and destination coefficients and all ROI fields.
It is not yet a file-to-file transcoder: decoding packets, emitting tile
headers and encoding the resulting coefficient stream still require the
future integrated session executors.
