# Managed entropy decoder contract

This contract defines the first managed JPEG XR decoder boundary. It covers only
the reference profile: 8-bit `Y_ONLY`, sequential spatial bitstream, QP=1,
one tile, no overlap and no alpha.

## Exact type mapping

| Reference C | Managed .NET Framework 2.0 |
| --- | --- |
| `U8` | `byte` |
| `I16` | `short` |
| `U16` | `ushort` |
| `I32`, `Int`, `PixelI` | `int` |
| `U32`, `UInt` | `uint` |
| `U8*` | `byte[]` plus explicit index |
| `PixelI*` | `int[]` plus explicit index |

No managed API may use pointers, `unsafe`, P/Invoke, platform word-size types,
or ambient global entropy state.

## Internal API shape

```text
sealed class JxrBitReader
    ReadBits(int count) -> uint

sealed class JxrAdaptiveHuffman
    DecodeSymbol(JxrBitReader reader) -> int
    Adapt()

sealed class JxrAdaptiveScan
    GetCoefficientIndex(int scanPosition) -> int
    ObserveNonZero(int scanPosition)

sealed class JxrEntropyContext
    DcModel, LpModel, AcModel
    LowpassScan, HorizontalScan, VerticalScan

DecodeDc(context, reader, coefficients)
DecodeLp(context, reader, coefficients)
DecodeHp(context, reader, coefficients, cbp)
```

Each method returns a decoder error value instead of throwing for malformed
bitstream input. An array access failure, insufficient input bit, invalid
Huffman symbol or out-of-range coefficient position is `InvalidBitstream`.

## Arithmetic rules

- Keep all entropy arithmetic in `int`/`uint`; do not use `long` implicitly.
- Replace C signed right shifts with explicit arithmetic-shift helper methods
  where the sign is relevant.
- Perform shifts only with validated counts in `0..31`.
- Preserve two's-complement sign reconstruction: `(value ^ sign) - sign`.
- Treat overflow as the same 32-bit wraparound used by the reference only where
  the reference relies on it; otherwise validate range before multiplication.

## Oracle artifacts

The managed implementation is accepted only when it matches:

- `minimal-profile/minimal-gray-16x16.jxr`;
- `minimal-profile/trace/encoder-bitstream.jxr`;
- DC bit range `1320..1330`, LP `1330..1544`, HP `1544..3502`;
- the native `JxrConformanceTests` suite.

The matching C migration adapters are `JxrManagedBitIO` and `JxrEntropyState`.
They intentionally expose buffer/index/state explicitly and form the direct
template for the managed classes above.
