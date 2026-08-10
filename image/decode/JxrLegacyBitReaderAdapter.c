#include "JxrLegacyBitReaderAdapter.h"
#include "JXRTrace.h"
#include "JxrPacketExecutor.h"
#include "JxrWmpPacketSource.h"

static U8* JxrLegacyBitReaderAdapterRingBuffer(BitIOInfo* stream)
{
    return (U8*)stream - PACKETLENGTH * 2;
}

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
        size_t legacyStartIndex = (state->inputBufferState.packetStartIndex + PACKETLENGTH) %
            state->inputBufferState.length;
        size_t legacyOffset = before.offRef + PACKETLENGTH;

        JxrWmpPacketSourceInit(&wmpSource, before.pWS, &source);
        JxrPacketExecutorTryRefill(&state->inputBufferState, &source,
            PACKETLENGTH, &executorDidRefill);
        state->stream->pbStart = state->inputBufferState.buffer +
            state->inputBufferState.packetStartIndex;
        state->stream->offRef = state->inputBufferState.streamOffset;
        state->stream->uiShadow = state->inputBufferState.shadow;
        executorMatchesLegacy = executorDidRefill && legacyNeedsRefill &&
            state->inputBufferState.packetStartIndex == legacyStartIndex &&
            state->inputBufferState.currentIndex ==
                (size_t)(before.pbCurrent - state->inputBufferState.buffer) &&
            state->inputBufferState.streamOffset == legacyOffset &&
            state->inputBufferState.shadow == *(U32*)before.pbStart;
        JXRTraceDumpRefillSnapshot(&before, state->stream, TRUE, legacyNeedsRefill,
            executorDidRefill, executorMatchesLegacy,
            (UINTPTR_T)(state->inputBufferState.buffer + state->inputBufferState.packetStartIndex),
            (UINTPTR_T)(state->inputBufferState.buffer + state->inputBufferState.currentIndex),
            state->inputBufferState.streamOffset,
            state->inputBufferState.shadow, wmpSource.lastReadResult);
        JxrLegacyBitReaderAdapterSyncInputBufferState(state);
    }
}

Void JxrLegacyBitReaderAdapterSyncInputBufferState(JxrLegacyBitReaderAdapter* state)
{
    U8* buffer = JxrLegacyBitReaderAdapterRingBuffer(state->stream);
    JxrBitInputBufferStateInit(&state->inputBufferState, buffer, PACKETLENGTH * 2,
        (size_t)(state->stream->pbStart - buffer),
        (size_t)(state->stream->pbCurrent - buffer), state->stream->offRef, state->stream->uiShadow);
    JxrBitCursorStateInitFromLegacy(&state->bitCursor, state->stream);
}

Bool JxrLegacyBitReaderAdapterIsInputBufferStateCurrent(const JxrLegacyBitReaderAdapter* state)
{
    return state->inputBufferState.buffer == JxrLegacyBitReaderAdapterRingBuffer(state->stream) &&
        state->inputBufferState.length == PACKETLENGTH * 2 &&
        state->inputBufferState.packetStartIndex ==
            (size_t)(state->stream->pbStart - state->inputBufferState.buffer) &&
        state->inputBufferState.currentIndex ==
            (size_t)(state->stream->pbCurrent - state->inputBufferState.buffer) &&
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
