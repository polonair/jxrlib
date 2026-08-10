#include "JxrSharedBitReaderState.h"
#include "JXRTrace.h"
#include "JxrPacketExecutor.h"
#include "JxrWmpPacketSource.h"
#include "JxrLegacyBitIoBridge.h"

static Void JxrSharedBitReaderStateRefreshIfExternalStateChanged(JxrSharedBitReaderState* state)
{
    if (!JxrLegacyBitIoBridgeCursorMatches(state->legacyStream, &state->bitCursor) ||
        !JxrLegacyBitIoBridgeInputMatches(state->legacyStream, &state->inputBufferState)) {
        JxrSharedBitReaderStateSyncFromLegacy(state);
    }
}

Void JxrSharedBitReaderStateInit(JxrSharedBitReaderState* state, BitIOInfo* legacyStream)
{
    state->legacyStream = legacyStream;
    JxrSharedBitReaderStateSyncFromLegacy(state);
}

U32 JxrSharedBitReaderStatePeek(JxrSharedBitReaderState* state, U32 count)
{
    JxrSharedBitReaderStateRefreshIfExternalStateChanged(state);
    return JxrBitCursorStatePeek(&state->bitCursor, count);
}

Void JxrSharedBitReaderStateConsume(JxrSharedBitReaderState* state, U32 count)
{
    JxrSharedBitReaderStateRefreshIfExternalStateChanged(state);
    JxrBitCursorStateConsume(&state->bitCursor, count);
    state->inputBufferState.currentIndex = state->bitCursor.currentIndex;
    JxrLegacyBitIoBridgeApplyCursor(state->legacyStream, &state->bitCursor);
}

U32 JxrSharedBitReaderStateReadLong(JxrSharedBitReaderState* state, U32 count)
{
    U32 value;
    JxrSharedBitReaderStateRefreshIfExternalStateChanged(state);
    value = JxrBitCursorStateReadLong(&state->bitCursor, count);
    state->inputBufferState.currentIndex = state->bitCursor.currentIndex;
    JxrLegacyBitIoBridgeApplyCursor(state->legacyStream, &state->bitCursor);
    return value;
}

Void JxrSharedBitReaderStateRefillLevel1(CWMImageStrCodec* codec, JxrSharedBitReaderState* state)
{
    UNREFERENCED_PARAMETER(codec);
    JxrSharedBitReaderStateSyncFromLegacy(state);
    if (JxrSharedBitReaderStateNeedsRefill(state)) {
        BitIOInfo before = *state->legacyStream;
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
        JxrLegacyBitIoBridgeApplyInput(state->legacyStream, &state->inputBufferState);
        executorMatchesLegacy = executorDidRefill && legacyNeedsRefill &&
            state->inputBufferState.packetStartIndex == legacyStartIndex &&
            state->inputBufferState.currentIndex ==
                (size_t)(before.pbCurrent - state->inputBufferState.buffer) &&
            state->inputBufferState.streamOffset == legacyOffset &&
            state->inputBufferState.shadow == *(U32*)before.pbStart;
        JXRTraceDumpRefillSnapshot(&before, state->legacyStream, TRUE, legacyNeedsRefill,
            executorDidRefill, executorMatchesLegacy,
            (UINTPTR_T)(state->inputBufferState.buffer + state->inputBufferState.packetStartIndex),
            (UINTPTR_T)(state->inputBufferState.buffer + state->inputBufferState.currentIndex),
            state->inputBufferState.streamOffset, state->inputBufferState.shadow,
            wmpSource.lastReadResult);
    }
}

Void JxrSharedBitReaderStateRefillLevel2(CWMImageStrCodec* codec, JxrSharedBitReaderState* state)
{
    UNREFERENCED_PARAMETER(codec);
    UNREFERENCED_PARAMETER(state);
}

Void JxrSharedBitReaderStateSyncFromLegacy(JxrSharedBitReaderState* state)
{
    JxrLegacyBitIoBridgeRead(state->legacyStream, &state->bitCursor, &state->inputBufferState);
}

Bool JxrSharedBitReaderStateIsCurrent(const JxrSharedBitReaderState* state)
{
    return JxrLegacyBitIoBridgeCursorMatches(state->legacyStream, &state->bitCursor) &&
        JxrLegacyBitIoBridgeInputMatches(state->legacyStream, &state->inputBufferState);
}

Bool JxrSharedBitReaderStateNeedsRefill(const JxrSharedBitReaderState* state)
{
    return JxrBitInputBufferStateNeedsRefill(&state->inputBufferState, PACKETLENGTH);
}

Bool JxrSharedBitReaderStateSharesStream(const JxrSharedBitReaderState* left,
    const JxrSharedBitReaderState* right)
{
    return left->legacyStream == right->legacyStream;
}
