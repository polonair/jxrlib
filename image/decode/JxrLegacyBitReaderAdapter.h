#ifndef JXR_LEGACY_BIT_READER_ADAPTER_H
#define JXR_LEGACY_BIT_READER_ADAPTER_H

#include "strcodec.h"
#include "JxrBitInputBufferState.h"
#include "JxrBitCursorState.h"

/* The only decoder-facing layer allowed to invoke legacy BitIO macros. */
typedef struct JxrLegacyBitReaderAdapter {
    BitIOInfo* stream;
    JxrBitInputBufferState inputBufferState;
    JxrBitCursorState bitCursor;
} JxrLegacyBitReaderAdapter;

Void JxrLegacyBitReaderAdapterInit(JxrLegacyBitReaderAdapter* state, BitIOInfo* stream);
U32 JxrLegacyBitReaderAdapterPeek16(JxrLegacyBitReaderAdapter* state, U32 count);
Void JxrLegacyBitReaderAdapterConsume16(JxrLegacyBitReaderAdapter* state, U32 count);
U32 JxrLegacyBitReaderAdapterRead32(JxrLegacyBitReaderAdapter* state, U32 count);
Void JxrLegacyBitReaderAdapterRefillLevel1(CWMImageStrCodec* codec, JxrLegacyBitReaderAdapter* state);
Void JxrLegacyBitReaderAdapterRefillLevel2(CWMImageStrCodec* codec, JxrLegacyBitReaderAdapter* state);
Void JxrLegacyBitReaderAdapterSyncInputBufferState(JxrLegacyBitReaderAdapter* state);
Bool JxrLegacyBitReaderAdapterIsInputBufferStateCurrent(const JxrLegacyBitReaderAdapter* state);
Bool JxrLegacyBitReaderAdapterNeedsRefill(const JxrLegacyBitReaderAdapter* state);
Bool JxrLegacyBitReaderAdapterHasMatchingRefillDecision(const JxrLegacyBitReaderAdapter* state);
Bool JxrLegacyBitReaderAdapterSharesStream(const JxrLegacyBitReaderAdapter* left,
    const JxrLegacyBitReaderAdapter* right);

#endif
