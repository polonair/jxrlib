# Managed module interfaces

This document defines the internal C# contracts used to port JPEG XR modules
one at a time.  It supplements `managed-entropy-decoder-contract.md` and
`managed-transcode-portability-contract.md`.  The contracts target C# 2.0 and
.NET Framework 2.0: no `unsafe`, P/Invoke, `Span<T>`, tasks, tuples, records,
or APIs introduced by later framework versions.

The contracts are internal implementation boundaries, not the future public
library API.  They are intentionally small so a C module can be ported and
compared with its native vectors before the next module is connected.

## Ownership and data-flow rules

There are four kinds of values crossing a module boundary:

| Kind | Examples | Ownership rule |
| --- | --- | --- |
| Configuration | `JxrImageConfiguration`, `JxrTileLayout` | Construct once from headers; read-only thereafter. |
| Mutable algorithm state | `JxrBitReaderState`, `JxrEntropyContext` | Owned by exactly one decoder or encoder session. |
| Buffer range | `JxrByteRange`, `JxrIntRange` | Array is owned by a session/plane; range never owns or reallocates it. |
| I/O boundary | `IJxrByteSource`, `IJxrByteSink`, `IJxrPixelSink` | Caller owns the object and controls its lifetime. |

Normal module methods receive all state they change as explicit parameters.
They must not read a global codec object or follow a hidden alpha-plane link.
An input array is never resized by a callee.  A module that needs new storage
returns it through an `out` result and the session takes ownership only after a
successful result.

Malformed file data returns `JxrError`; it is not represented by exceptions.
`ArgumentException`, `InvalidOperationException` and similar exceptions are
reserved for programmer misuse of an already constructed internal object.

## Common C# 2.0 skeletons

```csharp
namespace Jxr.Managed.Internal
{
    internal enum JxrError
    {
        None = 0,
        InvalidArgument,
        InvalidBitstream,
        UnexpectedEndOfStream,
        UnsupportedFeature,
        OutOfMemory,
        OutputFailure
    }

    // Describes a valid subsection; it does not own Buffer.
    internal sealed class JxrByteRange
    {
        internal byte[] Buffer;
        internal int Offset;
        internal int Count;
    }

    // Coefficients use int because PixelI maps to Int32.
    internal sealed class JxrIntRange
    {
        internal int[] Buffer;
        internal int Offset;
        internal int Count;
    }

    internal interface IJxrByteSource
    {
        JxrError Read(byte[] destination, int offset, int count,
            out int bytesRead);
    }

    internal interface IJxrByteSink
    {
        JxrError Write(byte[] source, int offset, int count);
    }

    internal interface IJxrPixelSink
    {
        JxrError WriteRow(int rowIndex, byte[] samples, int offset, int count);
    }
}
```

`Offset` and `Count` must be checked together, using checked `long`
intermediate arithmetic when validating `offset + count`.  A range is valid
only if its buffer is non-null and `0 <= Offset <= Buffer.Length` and
`0 <= Count <= Buffer.Length - Offset`.

`JxrBitReader` is the managed port of the standalone MSB-first reader in
`JxrManagedBitIO.c`.  It owns no stream: its input is one caller-owned
`byte[]`, and its explicit state is `ByteIndex`, `Accumulator`,
`BufferedBitCount`, `BitPosition` and `HasFailed`.  `ReadBits` accepts
zero through 32 bits and retains the native partial-consumption rule when EOF
is reached; `PeekBits` and `ConsumeBits` are managed-only, explicit helpers
needed by `AdaptiveHuffman`.  The `bit_reader_vectors` test executes the same
fields and EOF transition in the native and managed runners.

`JxrPacketReader` is the next, syntax-only layer.  It consumes four octets
from a `JxrBitReader` into `JxrPacketHeader`: two zero prefixes, marker `1`,
and the five-bit tile ID plus three-bit packet type.  It neither refills input
nor attaches decoder streams; those responsibilities remain with the future
packet transport module.  `packet_header_syntax_reader_vectors` covers valid,
invalid and truncated headers in both runners.

## Bitstream and entropy contracts

The lowest porting layer is independent of headers, image dimensions and pixel
output.  It accepts a byte source and exposes only a validated bit cursor.

`JxrBitMath` is a stateless prerequisite for this layer.  It exposes only
`RotateLeft32(uint value, uint count)` and `LowMask32(uint bitCount)`; both
return `uint`, normalize rotation counts modulo 32 and never use signed shifts.

```csharp
namespace Jxr.Managed.Internal
{
    internal sealed class JxrBitReaderState
    {
        internal byte[] Buffer;
        internal int BufferOffset;
        internal int BufferCount;
        internal int ByteIndex;
        internal uint Accumulator;
        internal int AvailableBitCount;
        internal bool EndOfInput;
    }

    internal sealed class JxrBitReader
    {
        internal JxrBitReader(IJxrByteSource source, JxrBitReaderState state) {}

        internal JxrError ReadBits(int bitCount, out uint value) { value = 0; return JxrError.None; }
        internal JxrError AlignToByte() { return JxrError.None; }
    }

    internal sealed class JxrAdaptiveHuffman
    {
        internal int Discriminant;

        internal JxrError DecodeSymbol(JxrBitReader reader, out int symbol)
        { symbol = 0; return JxrError.None; }

        // Returns the selected JPEG XR code and bit length for the encoder.
        internal JxrError GetCodeWord(int symbol, out uint code, out int bitCount)
        { code = 0; bitCount = 0; return JxrError.None; }

        internal void ObserveSymbol(int symbol) {}
        internal void Adapt() {}
    }

    internal sealed class JxrAdaptiveScan
    {
        internal JxrError ResetTotals(int count) { return JxrError.None; }
        internal JxrError GetCoefficientIndex(int scanPosition, out uint coefficientIndex)
        { coefficientIndex = 0; return JxrError.None; }
        internal JxrError ObserveNonZero(int scanPosition) { return JxrError.None; }
    }

    internal sealed class JxrEntropyContext
    {
        internal JxrAdaptiveModel DcModel;
        internal JxrAdaptiveModel LpModel;
        internal JxrAdaptiveModel AcModel;
        internal JxrAdaptiveScan LowpassScan;
        internal JxrAdaptiveScan HorizontalScan;
        internal JxrAdaptiveScan VerticalScan;
        internal JxrLowpassCbpState LowpassCbp;
        internal JxrCbpPredictionModel HighpassCbp;
        internal void Reset() {}
    }
}
```

The implemented `JxrEntropyContext` owns the three `CAdaptiveModel` equivalents,
three scan states, and LP/HP CBP counters.  `Reset` mirrors native
`JxrEntropyContextReset`: DC FLC bits become `(8,8)`, LP `(4,4)`, AC `(0,0)`;
LP CBP counters become `(1,1)` and HP CBP counts/states become
`(-4,4,0)` for each context.  It reinitializes scan indexes but preserves
scan totals and object identities.  Trim flex bits and ROI flags also survive.
The native and managed tests named `explicit_entropy_context` compare these
same reset transitions.  Decoder/encoder specific Huffman table adaptation
in `ResetCodingContextDec`/`ResetCodingContextEnc` is a separate integration
operation; those native functions perform more than `JxrEntropyContextReset`.

The `DcCodec`, `LpCodec` and `HpCodec` methods receive a reader, one explicit
entropy context, a coefficient range and a macroblock position.  They modify
only the supplied context and coefficients.  CBP values are passed and returned
as `uint`; signed coefficient values are `int`.

```csharp
internal static class JxrEntropyDecoder
{
    internal static JxrError DecodeDc(JxrBitReader reader,
        JxrEntropyContext context, JxrIntRange coefficients,
        int macroblockIndex) { return JxrError.None; }

    internal static JxrError DecodeLp(JxrBitReader reader,
        JxrEntropyContext context, JxrIntRange coefficients,
        int macroblockIndex) { return JxrError.None; }

    internal static JxrError DecodeHp(JxrBitReader reader,
        JxrEntropyContext context, JxrIntRange coefficients,
        int macroblockIndex, out uint codedBlockPattern)
    { codedBlockPattern = 0; return JxrError.None; }
}
```

## Coefficient, transform and pixel contracts

The entropy layer produces a plane of coefficients; it does not allocate
pixels.  Prediction, quantization and transforms work on explicit plane
buffers, which makes their native vectors directly reusable.

`CoefficientState` is the first managed component at this boundary.  It ports
`JxrCoefficientBuffer`, `JxrCoefficientPlaneState`, macroblock DC/quantizer
state and LP/HP CBP state.  Pointer-plus-length views become
`JxrCoefficientBuffer(int[] values, int offset, int count)`.  A caller-owned
array is never implicitly mutated through an alias: `JxrMacroblockSnapshot`
and `JxrMacroblockCbpState` use explicit `LoadFrom` and `CopyTo` operations.
This is the managed replacement for the temporary native load/commit bridge.

```csharp
namespace Jxr.Managed.Core
{
    internal sealed class JxrCoefficientBuffer
    {
        internal JxrError Get(int index, out int value) { value = 0; return JxrError.None; }
        internal JxrError Set(int index, int value) { return JxrError.None; }
        internal JxrError Add(int index, int value) { return JxrError.None; }
        internal void Clear() {}
    }

    internal sealed class JxrCoefficientPlaneState
    {
        internal JxrError GetBlock(int plane, int offset, int count,
            out JxrCoefficientBuffer block)
        { block = null; return JxrError.None; }
    }

    internal sealed class JxrMacroblockState
    {
        internal JxrError LoadFrom(JxrMacroblockSnapshot source) { return JxrError.None; }
        internal JxrError CopyTo(JxrMacroblockSnapshot destination) { return JxrError.None; }
        internal JxrError ClearDc(int channelCount) { return JxrError.None; }
    }

    internal sealed class JxrMacroblockCbpState
    {
        internal JxrError LoadFrom(int[] cbp, int[] differential) { return JxrError.None; }
        internal JxrError CopyTo(int[] cbp, int[] differential) { return JxrError.None; }
    }
}
```

`JxrLowpassCbpState` owns its two adaptive counters, while
`JxrHighpassCbpState` explicitly groups pattern Huffman state, count Huffman
state and `JxrCbpPredictionModel`.  The reference vectors named
`coefficient_buffer_vectors`, `coefficient_plane_state_vectors`,
`macroblock_state_vectors`, `macroblock_cbp_state_vectors`,
`lowpass_cbp_state_vectors` and `highpass_cbp_state_vectors` run in both the
native and managed runners.

```csharp
namespace Jxr.Managed.Internal
{
    internal sealed class JxrCoefficientPlane
    {
        internal int[] Values;
        internal int ChannelCount;
        internal int MacroblockCount;
        internal int CoefficientsPerMacroblock;
    }

    internal sealed class JxrQuantizerSet
    {
        internal int[] Dc;
        internal int[] Lowpass;
        internal int[] Highpass;
    }

    internal sealed class JxrPixelPlane
    {
        internal byte[] Samples;
        internal int Width;
        internal int Height;
        internal int Stride;
        internal int ChannelCount;
    }

    internal static class JxrInverseTransform
    {
        internal static JxrError TransformMacroblock(
            JxrCoefficientPlane coefficients, JxrQuantizerSet quantizers,
            int macroblockIndex, JxrPixelPlane destination)
        { return JxrError.None; }
    }
}
```

The future encoder uses the same `JxrPixelPlane` and `JxrCoefficientPlane`
types in reverse: input conversion and forward transform fill coefficients,
then the entropy encoder writes to an `IJxrByteSink`.

## Header, tile and session contracts

Headers create immutable configuration.  Packet and transform modules consume
that configuration rather than re-reading or altering it.

```csharp
namespace Jxr.Managed.Internal
{
    internal sealed class JxrImageConfiguration
    {
        internal int Width;
        internal int Height;
        internal int ChannelCount;
        internal int BitsPerSample;
        internal bool HasAlpha;
        internal bool IsFrequencyMode;
    }

    internal sealed class JxrTileLayout
    {
        internal int TileColumnCount;
        internal int TileRowCount;
        internal int[] ColumnWidths;
        internal int[] RowHeights;
    }

    internal sealed class JxrPlanePair
    {
        internal JxrCoefficientPlane Primary;
        internal JxrCoefficientPlane Alpha;
    }

    internal sealed class JxrDecoderSession
    {
        internal JxrImageConfiguration Configuration;
        internal JxrTileLayout TileLayout;
        internal JxrPlanePair Planes;
        internal JxrEntropyContext Entropy;
    }

    internal static class JxrHeaderReader
    {
        internal static JxrError ReadConfiguration(JxrBitReader reader,
            out JxrImageConfiguration configuration, out JxrTileLayout tileLayout)
        { configuration = null; tileLayout = null; return JxrError.None; }
    }
}
```

`JxrDecoderSession` is the only owner that composes configuration, entropy
state and coefficient planes.  It is constructed only after headers validate.
The alpha field is either `null` or a separately constructed plane; no managed
type represents the native `m_pNextSC` linked list.

## Module dependency order

```text
BitMath
  ├─ AdaptiveHuffman ─┐
  ├─ AdaptiveScan ────┼─ EntropyContext ─ DC / LP / HP
  └─ BitReader ───────┘                    │
                                           CoefficientPlane
                                                │
HeaderReader ─ ImageConfiguration / TileLayout ─┼─ Quantization / Prediction
                                                │              │
                                                └─ InverseTransform ─ PixelPlane ─ IPixelSink
```

Port modules in topological order.  A managed module is eligible to be joined
to the next layer only after its C# tests are direct translations of the named
native vectors and both implementations pass them.

## Native oracle mapping

| Managed module | Native vector groups to translate first |
| --- | --- |
| `JxrAdaptiveHuffman` | `huffman_state_set_vectors`, `huffman_decoder_vectors`, `adaptive_model_state_vectors` |
| `JxrAdaptiveScan` | `adaptive_scan_vectors`, `adaptive_scan_state_vectors` |
| `JxrBitReader` | `bit_reader_core_vectors`, `bit_cursor_state_vectors`, `bit_cursor_ring_wrap_vectors`, `bit_input_buffer_state_vectors` |
| `JxrEntropyDecoder` | `explicit_entropy_context`, `entropy_reader_state_vectors`, `dc_conformance`, `lp_conformance`, `hp_conformance` |
| coefficient/transform types | predictor, dequantizer, forward and inverse transform vector groups |
| headers and sessions | header, packet, memory-layout and session vector groups |

Integration is accepted only after the managed result also matches the
minimal-profile JXR/BMP fixtures and their trace artifacts described by the two
existing managed contracts.
