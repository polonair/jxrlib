#include "JxrLegacyBitReaderAdapter.h"
#include "JXRTrace.h"
#include "JxrPacketExecutor.h"
#include "JxrWmpPacketSource.h"

Void JxrLegacyBitReaderAdapterInit(JxrLegacyBitReaderAdapter* state, BitIOInfo* stream)
{
    state->stream = stream;
    JxrLegacyBitReaderAdapterSyncInputBufferState(state);
}

U32 JxrLegacyBitReaderAdapterPeek16(JxrLegacyBitReaderAdapter* state, U32 count)
{
    JxrBitCursorStateInitFromLegacy(&state->bitCursor, state->stream);
    return JxrBitCursorStatePeek(&state->bitCursor, count);
}

Void JxrLegacyBitReaderAdapterConsume16(JxrLegacyBitReaderAdapter* state, U32 count)
{
    JxrBitCursorStateInitFromLegacy(&state->bitCursor, state->stream);
    JxrBitCursorStateConsume(&state->bitCursor, count);
    state->stream->pbCurrent = (U8*)state->bitCursor.buffer + state->bitCursor.currentIndex;
    state->stream->uiAccumulator = state->bitCursor.accumulator;
    state->stream->cBitsUsed = state->bitCursor.usedBits;
    JxrLegacyBitReaderAdapterSyncInputBufferState(state);
}

U32 JxrLegacyBitReaderAdapterRead32(JxrLegacyBitReaderAdapter* state, U32 count)
{
    U32 value;
    JxrBitCursorStateInitFromLegacy(&state->bitCursor, state->stream);
    value = JxrBitCursorStateReadLong(&state->bitCursor, count);
    state->stream->pbCurrent = (U8*)state->bitCursor.buffer + state->bitCursor.currentIndex;
    state->stream->uiAccumulator = state->bitCursor.accumulator;
    state->stream->cBitsUsed = state->bitCursor.usedBits;
    JxrLegacyBitReaderAdapterSyncInputBufferState(state);
    return value;
}

Void JxrLegacyBitReaderAdapterRefillLevel1(CWMImageStrCodec* codec, JxrLegacyBitReaderAdapter* state)
{
    UNREFERENCED_PARAMETER(codec);
    /* Direct legacy consumers may have advanced pbCurrent since the last reader call. */
    JxrLegacyBitReaderAdapterSyncInputBufferState(state);
    if (JxrLegacyBitReaderAdapterNeedsRefill(state)) {
        BitIOInfo before = *state->stream;
        JxrPacketSource source;
        JxrWmpPacketSource wmpSource;
        Bool executorDidRefill;
        Bool executorMatchesLegacy;
        Bool legacyNeedsRefill = ((((INTPTR_T)before.pbStart ^
            (INTPTR_T)before.pbCurrent) & (UINTPTR_T)PACKETLENGTH) != 0);
        UINTPTR_T legacyStartAddress = (UINTPTR_T)MASKPTR(before.pbStart + PACKETLENGTH, before.iMask);
        size_t legacyOffset = before.offRef + PACKETLENGTH;

        JxrWmpPacketSourceInit(&wmpSource, before.pWS, &source);
        JxrPacketExecutorTryRefill(&state->inputBufferState, &source, before.pbStart,
            PACKETLENGTH, &executorDidRefill);
        state->stream->pbStart = (U8*)state->inputBufferState.startAddress;
        state->stream->offRef = state->inputBufferState.streamOffset;
        state->stream->uiShadow = state->inputBufferState.shadow;
        executorMatchesLegacy = executorDidRefill && legacyNeedsRefill &&
            state->inputBufferState.startAddress == legacyStartAddress &&
            state->inputBufferState.currentAddress == (UINTPTR_T)before.pbCurrent &&
            state->inputBufferState.streamOffset == legacyOffset &&
            state->inputBufferState.shadow == *(U32*)before.pbStart;
        JXRTraceDumpRefillSnapshot(&before, state->stream, TRUE, legacyNeedsRefill,
            executorDidRefill, executorMatchesLegacy, state->inputBufferState.startAddress,
            state->inputBufferState.currentAddress, state->inputBufferState.streamOffset,
            state->inputBufferState.shadow, wmpSource.lastReadResult);
        JxrLegacyBitReaderAdapterSyncInputBufferState(state);
    }
}

Void JxrLegacyBitReaderAdapterSyncInputBufferState(JxrLegacyBitReaderAdapter* state)
{
    JxrBitInputBufferStateInit(&state->inputBufferState,
        (UINTPTR_T)state->stream->pbStart, (UINTPTR_T)state->stream->pbCurrent,
        (UINTPTR_T)state->stream->iMask, state->stream->offRef, state->stream->uiShadow);
    JxrBitCursorStateInitFromLegacy(&state->bitCursor, state->stream);
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
