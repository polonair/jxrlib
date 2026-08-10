#include "JxrLegacyBitReaderAdapter.h"

Void JxrLegacyBitReaderAdapterInit(JxrLegacyBitReaderAdapter* state, BitIOInfo* stream)
{
    JxrSharedBitReaderStateInit(state, stream);
}

U32 JxrLegacyBitReaderAdapterPeek16(JxrLegacyBitReaderAdapter* state, U32 count)
{
    return JxrSharedBitReaderStatePeek(state, count);
}

Void JxrLegacyBitReaderAdapterConsume16(JxrLegacyBitReaderAdapter* state, U32 count)
{
    JxrSharedBitReaderStateConsume(state, count);
}

U32 JxrLegacyBitReaderAdapterRead32(JxrLegacyBitReaderAdapter* state, U32 count)
{
    return JxrSharedBitReaderStateReadLong(state, count);
}

Bool JxrLegacyBitReaderAdapterRefillLevel1(CWMImageStrCodec* codec, JxrLegacyBitReaderAdapter* state)
{
    return JxrSharedBitReaderStateRefillLevel1(codec, state);
}

Void JxrLegacyBitReaderAdapterSyncInputBufferState(JxrLegacyBitReaderAdapter* state)
{
    JxrSharedBitReaderStateSyncFromLegacy(state);
}

Bool JxrLegacyBitReaderAdapterIsInputBufferStateCurrent(const JxrLegacyBitReaderAdapter* state)
{
    return JxrSharedBitReaderStateIsCurrent(state);
}

Bool JxrLegacyBitReaderAdapterNeedsRefill(const JxrLegacyBitReaderAdapter* state)
{
    return JxrSharedBitReaderStateNeedsRefill(state);
}

Bool JxrLegacyBitReaderAdapterHasMatchingRefillDecision(const JxrLegacyBitReaderAdapter* state)
{
    Bool legacyNeedsRefill = ((((INTPTR_T)state->legacyStream->pbStart ^
        (INTPTR_T)state->legacyStream->pbCurrent) & (UINTPTR_T)PACKETLENGTH) != 0);
    return legacyNeedsRefill == JxrLegacyBitReaderAdapterNeedsRefill(state);
}

Bool JxrLegacyBitReaderAdapterRefillLevel2(CWMImageStrCodec* codec, JxrLegacyBitReaderAdapter* state)
{
    return JxrSharedBitReaderStateRefillLevel2(codec, state);
}

Bool JxrLegacyBitReaderAdapterSharesStream(const JxrLegacyBitReaderAdapter* left,
    const JxrLegacyBitReaderAdapter* right)
{
    return JxrSharedBitReaderStateSharesStream(left, right);
}
