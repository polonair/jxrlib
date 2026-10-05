# Managed JPEG XR port

The [compatibility baseline](../analysis/managed-compatibility-matrix.md)
separates complete file conversions from independently ported codec stages
and maps both to the native reference fixtures.

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

## Public pixel and stream API

`JxrImage` describes top-down, row-major pixels with explicit width, height,
format and byte stride. `Gray8` is integrated end-to-end. `Rgb24` and `Bgr24`
select R-G-B and B-G-R byte order respectively. Both encode and decode support
8-bit YUV 4:4:4, 4:2:2 and 4:2:0 in spatial packets with overlap levels
0, 1 and 2. The encoder can split the macroblock grid into soft-boundary
tiles through `JxrEncoderOptions.TileLayout`; column widths and row heights
are supplied in macroblocks, and packets are emitted in row-major tile order.
With no layout (or a one-tile layout), the existing untiled stream is retained.
The caller owns the image's `byte[]` buffer, which is not copied by the
constructor.

`JxrCodec.Encode(image, options, out byte[])` and
`JxrCodec.Decode(byte[], options, out image)` are BMP-independent. Matching
`Stream` overloads read or write JPEG XR without seeking or closing the
caller's stream. This stage buffers one complete compressed image internally;
it is not incremental streaming. A failed destination write may leave a
partial JXR in that stream and returns `IoFailure`.

The integrated `Gray8` encoder supports QP indexes 0–255 (indexes 0 and 1
select the native lossless index), independent DC/LP/HP QP overrides, all four
subband modes, optional flexbit trimming, overlap levels 0–2 and spatial layout.
Native fixture tests compare complete JXR bytes, quantized/predicted
coefficients and exact per-band bit boundaries for QP 1/2/16/64/255,
trim-flexbits, no-flexbits, no-HP and DC-only cases. The managed decoder also
matches native-decoded pixels for every tested mode, including no-flexbits.
`gray_mixed_qp_options` checks independent DC/LP/HP QP fields and managed
decoding; mixed-QP byte-for-byte C comparison is not covered by the native
command-line encoder, which exposes one image QP.
The color encoder supports the same four subband modes, QP fields and flexbit
trimming. It processes arbitrary image dimensions with replicated border
samples, luma and either full-resolution or subsampled chroma planes, shared entropy adaptation,
and per-channel prediction state. `rgb444_encoder_native_fixture` checks the
lossless real sign image against the native JXR byte-for-byte;
`rgb444_encoder_quality_native_fixtures` does the same for six lossy sign/city
profiles. `rgb444_sizes_round_trip` covers 1x1, 15x17, 16x16, 31x19 and
32x32, including a byte-identical native 15x17 reference. The subsampled
encoder applies the native five-tap chroma downsampler, compact transforms,
LP/HP entropy syntax and CBP packing; decoding interpolates chroma before
the inverse color transform. Native fixtures compare byte-identical JXR and
restored RGB pixels for 4:2:2/4:2:0, overlap levels 0–2, scaled and unscaled
arithmetic, single-macroblock and non-multiple-of-16 boundaries. The C encoder
rejects OL_TWO on a one-macroblock-wide subsampled image, and so does this API.
BGR input produces
the same JXR as RGB input with equivalent pixels. The BMP adapter also reads
and writes canonical 24bpp RGB/BGR images.
The native `transform_coefficients` trace is captured before the delayed
macroblock transform finishes; it is not the same snapshot as the managed
pre-quantization array, so conformance compares the native quantized and
predicted stages instead. Invalid option values return `InvalidArgument`;
valid but unimplemented profiles return `UnsupportedFeature`.
`JxrBmpAdapter` reads/writes canonical 8bpp Gray BMPs
at arbitrary dimensions and is optional: JPEG XR encoding and decoding no longer depend
on BMP. The original `JxrMinimalEncoder.EncodeGrayBmp` and
`JxrMinimalDecoder.DecodeGrayBmp` methods remain compatibility wrappers.
`public_pixel_api` and `public_stream_api` verify this boundary, including
padded image stride and non-seekable streams.

`JxrMinimalDecoder.DecodeGrayBmp` connects the ported modules into a
fully managed decoder for the supported 8-bit Y-only profile.
It accepts a JXR container or raw codestream, locates spatial packets from the
header and index table, decodes each tile with independent adaptive and
prediction state, applies dequantization
and inverse transform, and emits an 8-bit grayscale BMP. The
`minimal_decoder_end_to_end` test compares the entire BMP byte-for-byte with
both the source and native-decoded fixture. The RGB24 decode path additionally
handles spatial 8-bit YUV 4:4:4/4:2:2/4:2:0 streams, sharing DC/LP/HP entropy
and prediction state across Y, U and V within each tile. It inverse-transforms
the luma and full-resolution or subsampled chroma planes, then applies the
reversible color transform.
`real_rgb444_decode` checks the 334x330 real-image fixture against its source
BGR BMP. `rgb444_quality_native_fixtures` also compares native-restored RGB
pixels for QP=16 and four subband/trim variants of the sign image plus the
605x478 city image. The scaled-arithmetic chroma DC/LP values are doubled
after inverse transform stage 2, as in the native decoder. Corpus coverage
also includes single-tile spatial PPTX profiles with 8-bit RGB/YUV 4:4:4,
QP 15/30, `OL_ONE`, and planar alpha. Their spatial packet length escape and
shared chroma quantizer are handled. The planar-alpha sample matches native
alpha exactly; spatial color still has a tracked subversion-0/`OL_ONE`
compatibility gap (see `tools/pptx-jxr-corpus/known-mismatches-spatial.jsonl`).
Other JPEG XR profiles remain unsupported; this is not yet a general-purpose
JPEG XR decoder.

`JxrMinimalEncoder.EncodeGrayBmp` now connects the managed forward transform,
quantization, coefficient prediction, adaptive entropy state and bit writer.
It accepts a canonical 8-bit grayscale BMP (including the grayscale
palette and 3779 px/m resolution), and writes the corresponding lossless
Y-only JXR. `JxrHeaderWriter`, `JxrPacketWriter` and
`JxrContainerWriter` now serialize the JPEG XR header, null index-table
record, spatial packet header and TIFF-like container field by field using
`JxrBitWriter`. The container's codestream length is calculated from the
actual encoded packet. All coefficient and entropy data are computed from the
input pixels.
The single-tile spatial packet has no separate length field: the null
index-table record contains its own variable-length marker size, while the
container's `ImageByteCount` gives the codestream length. Tiled output instead
uses the index table to locate each packet.
The fixture JXR is reproduced byte-for-byte, and eighteen additional sample
patterns round-trip through the managed encoder and decoder. Image dimensions
need not be multiples of 16: border samples are replicated for encoding and
cropped after decoding. Consecutive macroblocks share entropy state, DC/LP
prediction rows and HP CBP neighbours. Spatial Gray/RGB decoding also
supports indexed soft-boundary tiles, including 4:2:2/4:2:0 and overlap.
`spatial_tile_encode_native_fixtures` compares tiled output byte-for-byte with
the C encoder across Gray/RGB formats, quality levels, overlap modes, uneven
tile widths and partial edge macroblocks. Frequency mode now emits separate
DC/LP/HP/flexbits packets and an index table; packet order can be progressive
(the default) or sequential. `frequency_layout_native_fixtures` compares full
JXR files byte-for-byte with C references for Gray and RGB 4:4:4/4:2:2/4:2:0,
single- and multi-tile streams, QP16, DC-only/no-highpass profiles, skipped
flexbits, flexbit trimming and sequential ordering. The managed decoder reads those frequency packets and
matches native-decoded pixels. Frequency-layout context reset and general
final flushing remain outside the integrated session executor.
`gray_sizes_native_fixtures` compares native and managed output for eight
dimensions, including a 17-macroblock-wide image that crosses an adaptive
scan reset boundary. The [fixture generator](fixtures/README.md) documents the
reference BMP/JXR pairs.
`header_writer_fixture` compares the generated header, codestream and full
container to the C fixture. `header_writer_fields` verifies changed dimensions,
quantizer indices, packet fields and lengths by parsing the generated output.

`JxrImagePipeline` currently covers the full-resolution 8-bit pixel boundary:
Gray/RGB input centering, reversible RGB/CMYK color transforms, scaled and
unscaled RGB/Gray output, clipping, and explicit row strides. Its native and
managed reference vectors share a frozen signature. Alpha and other
not-yet-integrated profiles remain future work.

`JxrDecoderSession` and `JxrEncoderSession` now own separate two-row
coefficient buffers for each channel and optional alpha plane. Their memory
plans reproduce the native decoder/encoder layout formulas, including the
32-bit safety decisions, while actual managed allocations are `int[]` rather
than a native struct-and-pointer slab. Sessions can be configured from parsed
JXR headers and release their buffers through `Dispose`. The minimal decoder
and encoder now execute spatial tiles and frequency packets through those
coefficient and packet paths; general final flushing remains outside this
integration.

`JxrTranscoder` ports the coefficient-domain core: all eight orientations,
DC/AC sign and position changes for 4:4:4, 4:2:2 and 4:2:0, and ROI expansion
to macroblock boundaries with overlap. The native and managed vector suites
compare both mutated source and destination coefficients and all ROI fields.
It is not yet a file-to-file transcoder: decoding packets, emitting tile
headers and encoding the resulting coefficient stream still require the
future integrated session executors.
