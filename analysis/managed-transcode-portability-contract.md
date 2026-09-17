# Managed transcode portability contract

This document records the managed-port boundary for the JPEG XR transcode
path.  The native C implementation remains the behavioural oracle; the future
implementation is a fully managed C# .NET Framework 2.0 implementation and
must not use P/Invoke or `unsafe` code.

## Scope and result

The transcode path has been decomposed into explicit modules for session
lifecycle, decoder and encoder initialization, alpha-plane ownership, frame
buffers, macroblock layout and processing, ROI geometry, orientation, tile
extraction, tile headers and quantizers.  The modules communicate through
explicit state instead of relying on a hidden secondary-plane traversal during
normal processing.

The following native modules are direct shapes for future managed types:

| Native module | Managed shape | Managed representation |
| --- | --- | --- |
| `JxrTranscodeSession` / `JxrTranscodeSessionRunner` | `TranscodeSession` | sealed class with `Dispose` and one `try/finally` owner |
| `JxrTranscodePlanePair` | `TranscodePlanePair` | primary plane plus optional alpha plane |
| `JxrTranscodePlaneBuffers` / `JxrTranscodeFrameBufferAllocator` | `PlaneBuffers` | `int[]` coefficient storage and explicit counts |
| `JxrTranscodeMacroblockBufferLayout` | `MacroblockBufferLayout` | coefficient count and per-channel integer offsets |
| `JxrTranscodeBitIoHeaderLayout` | `BitIoHeaderLayout` | `byte[]` allocation length and byte offsets |
| `JxrTranscodeSecondaryPlaneSetup` / `JxrTranscodeSecondaryPlaneLink` | `SecondaryPlaneSetup` | explicit object construction and ownership |
| `JxrTranscodeRoiGeometry` / `JxrTranscodeRoiTileLayout` | `RoiGeometry` | integer bounds, dimensions and tile positions |
| `JxrTranscodeOrientationState` / oriented macroblock modules | `OrientationState` | integer coordinate transform and arrays |
| `JxrTranscodeTileExtractionRequest` and executor | `TileExtractionRequest` | plane pair, ROI and destination supplied as parameters |
| macroblock processing, transform and encoder modules | `MacroblockProcessor` | input arrays, explicit offsets and context objects |

## Required managed rules

The managed core must use the type mapping already established by
`managed-entropy-decoder-contract.md`: `U8` is `byte`, `I16` is `short`,
`U16` is `ushort`, `I32`/`Int`/`PixelI` is `int`, and `U32`/`UInt` is `uint`.
Use `int[]` and `byte[]` with explicit indexes in place of pointer ranges.
Use `long` only for checked byte-size calculations before allocating an array;
validate the result before converting it to an `int` array length.

Every managed operation must receive its ownership and state explicitly:

- no pointer arithmetic, pointer casts, `unsafe`, P/Invoke or platform-word
  sized types;
- no macro-based alignment or hidden structure-layout assumptions;
- no shallow copy of a codec/session object to construct an alpha plane;
- no navigation through `m_pNextSC` outside the compatibility boundary;
- no global mutable codec state as an implicit input; and
- no unmanaged allocation or manual cleanup in the managed core.

Malformed inputs must be reported through the decoder/transcoder error model,
after validating array bounds, byte counts, tile counts and arithmetic shift
counts.  Lifecycle cleanup is the managed equivalent of the native session's
single cleanup path: every allocated managed object is owned by one session or
request object and is released by normal garbage collection after the session
is disposed.

## Native compatibility boundary

The following constructs are intentionally retained in C only because the
reference codec ABI still requires them.  They are adapters, not templates for
the managed implementation:

- `malloc`/`free` in session, frame-buffer, runtime-initializer, output and
  tile-index allocation modules;
- `CWMImageStrCodec` pointer fields and the final binding of `p1MBbuffer[]` to
  coefficient storage in `JxrTranscodeMacroblockBufferLayout`;
- `m_pNextSC` while creating, attaching, resolving or releasing a secondary
  alpha codec (`JxrTranscodePlanePair` is the normal processing representation);
- the native `BitIOInfo` layout and the byte allocation which backs it; and
- legacy stream callbacks and codec entry points at the outermost native
  adapter.

New native refactors must move logic toward the explicit modules above, rather
than adding a new dependency on one of these compatibility mechanisms.

## Verification contract

The native build and conformance runner remain the executable oracle for each
managed-port step.  In particular, the transcode contract is covered by:

- `transcode_session_release_contract_vectors`;
- `transcode_macroblock_buffer_layout_vectors`;
- `transcode_bit_io_header_layout_vectors`;
- `transcode_secondary_plane_setup_vectors` and
  `transcode_secondary_plane_link_vectors`;
- `transcode_plane_pair_vectors`, `transcode_plane_buffers_vectors` and
  `transcode_tile_extraction_executor_vectors`;
- `transcode_macroblock_processing_pipeline_contract_vectors`; and
- the `minimal_fixture`, `minimal_round_trip`, `real_image_round_trip` and
  `default_image_round_trip` integration fixtures.

The fixture and entropy-trace tests remain required because the transcode path
must preserve the existing decoder and encoder bitstream semantics.  The
untracked `csharp/` directory is not part of this native contract or its build.
