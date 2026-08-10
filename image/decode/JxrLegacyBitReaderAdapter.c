#include "JxrLegacyBitReaderAdapter.h"
#include "JXRTrace.h"

Void JxrLegacyBitReaderAdapterInit(JxrLegacyBitReaderAdapter* state, BitIOInfo* stream)
{
    state->stream = stream;
    JxrLegacyBitReaderAdapterSyncInputBufferState(state);
}

U32 JxrLegacyBitReaderAdapterPeek16(JxrLegacyBitReaderAdapter* state, U32 count)
{
    return peekBit16(state->stream, count);
}

Void JxrLegacyBitReaderAdapterConsume16(JxrLegacyBitReaderAdapter* state, U32 count)
{
    flushBit16(state->stream, count);
    JxrLegacyBitReaderAdapterSyncInputBufferState(state);
}

U32 JxrLegacyBitReaderAdapterRead32(JxrLegacyBitReaderAdapter* state, U32 count)
{
    U32 value = getBit32(state->stream, count);
    JxrLegacyBitReaderAdapterSyncInputBufferState(state);
    return value;
}

Void JxrLegacyBitReaderAdapterRefillLevel1(CWMImageStrCodec* codec, JxrLegacyBitReaderAdapter* state)
{
    assert(JxrLegacyBitReaderAdapterHasMatchingRefillDecision(state));
    if (JxrLegacyBitReaderAdapterNeedsRefill(state)) {
        BitIOInfo before = *state->stream;
        Bool legacyNeedsRefill = ((((INTPTR_T)before.pbStart ^
            (INTPTR_T)before.pbCurrent) & (UINTPTR_T)PACKETLENGTH) != 0);
        readIS(codec, state->stream);
        JXRTraceDumpRefillSnapshot(&before, state->stream, TRUE, legacyNeedsRefill);
        JxrLegacyBitReaderAdapterSyncInputBufferState(state);
    }
}

Void JxrLegacyBitReaderAdapterSyncInputBufferState(JxrLegacyBitReaderAdapter* state)
{
    JxrBitInputBufferStateInit(&state->inputBufferState,
        (UINTPTR_T)state->stream->pbStart, (UINTPTR_T)state->stream->pbCurrent,
        (UINTPTR_T)state->stream->iMask, state->stream->offRef, state->stream->uiShadow);
}

Bool JxrLegacyBitReaderAdapterIsInputBufferStateCurrent(const JxrLegacyBitReaderAdapter* state)
{
    return state->inputBufferState.startAddress == (UINTPTR_T)state->stream->pbStart &&
        state->inputBufferState.currentAddress == (UINTPTR_T)state->stream->pbCurrent &&
        state->inputBufferState.mask == (UINTPTR_T)state->stream->iMask &&
        state->inputBufferState.streamOffset == state->stream->offRef &&
        state->inputBufferState.shadow == state->stream->uiShadow;
}

Bool JxrLegacyBitReaderAdapterNeedsRefill(const JxrLegacyBitReaderAdapter* state)
{
    return JxrBitInputBufferStateNeedsRefill(&state->inputBufferState, PACKETLENGTH);
}

Bool JxrLegacyBitReaderAdapterHasMatchingRefillDecision(const JxrLegacyBitReaderAdapter* state)
{
    Bool legacyNeedsRefill = ((((INTPTR_T)state->stream->pbStart ^
        (INTPTR_T)state->stream->pbCurrent) & (UINTPTR_T)PACKETLENGTH) != 0);
    return legacyNeedsRefill == JxrLegacyBitReaderAdapterNeedsRefill(state);
}

Void JxrLegacyBitReaderAdapterRefillLevel2(CWMImageStrCodec* codec, JxrLegacyBitReaderAdapter* state)
{
    UNREFERENCED_PARAMETER(codec);
    UNREFERENCED_PARAMETER(state);
}

Bool JxrLegacyBitReaderAdapterSharesStream(const JxrLegacyBitReaderAdapter* left,
    const JxrLegacyBitReaderAdapter* right)
{
    return left->stream == right->stream;
}
