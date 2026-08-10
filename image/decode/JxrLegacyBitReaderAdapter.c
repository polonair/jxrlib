#include "JxrLegacyBitReaderAdapter.h"
#include "JXRTrace.h"
#include "JxrPacketExecutor.h"
#include "JxrWmpPacketSource.h"
#include "JxrLegacyBitIoBridge.h"

static Void JxrLegacyBitReaderAdapterRefreshIfExternalStateChanged(JxrLegacyBitReaderAdapter* state)
{
    if (!JxrLegacyBitIoBridgeCursorMatches(state->stream, &state->bitCursor) ||
        !JxrLegacyBitIoBridgeInputMatches(state->stream, &state->inputBufferState)) {
        JxrLegacyBitIoBridgeRead(state->stream, &state->bitCursor, &state->inputBufferState);
    }
}

Void JxrLegacyBitReaderAdapterInit(JxrLegacyBitReaderAdapter* state, BitIOInfo* stream)
{
    state->stream = stream;
    JxrLegacyBitReaderAdapterSyncInputBufferState(state);
}

U32 JxrLegacyBitReaderAdapterPeek16(JxrLegacyBitReaderAdapter* state, U32 count)
{
    JxrLegacyBitReaderAdapterRefreshIfExternalStateChanged(state);
    return JxrBitCursorStatePeek(&state->bitCursor, count);
}

Void JxrLegacyBitReaderAdapterConsume16(JxrLegacyBitReaderAdapter* state, U32 count)
{
    JxrLegacyBitReaderAdapterRefreshIfExternalStateChanged(state);
    JxrBitCursorStateConsume(&state->bitCursor, count);
    state->inputBufferState.currentIndex = state->bitCursor.currentIndex;
    JxrLegacyBitIoBridgeApplyCursor(state->stream, &state->bitCursor);
}

U32 JxrLegacyBitReaderAdapterRead32(JxrLegacyBitReaderAdapter* state, U32 count)
{
    U32 value;
    JxrLegacyBitReaderAdapterRefreshIfExternalStateChanged(state);
    value = JxrBitCursorStateReadLong(&state->bitCursor, count);
    state->inputBufferState.currentIndex = state->bitCursor.currentIndex;
    JxrLegacyBitIoBridgeApplyCursor(state->stream, &state->bitCursor);
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
        JxrLegacyBitIoBridgeApplyInput(state->stream, &state->inputBufferState);
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
    }
}

Void JxrLegacyBitReaderAdapterSyncInputBufferState(JxrLegacyBitReaderAdapter* state)
{
    JxrLegacyBitIoBridgeRead(state->stream, &state->bitCursor, &state->inputBufferState);
}

Bool JxrLegacyBitReaderAdapterIsInputBufferStateCurrent(const JxrLegacyBitReaderAdapter* state)
{
    return JxrLegacyBitIoBridgeCursorMatches(state->stream, &state->bitCursor) &&
        JxrLegacyBitIoBridgeInputMatches(state->stream, &state->inputBufferState);
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
