#include "JxrSharedBitReaderState.h"
#include "JXRTrace.h"
#include "JxrPacketExecutor.h"
#include "JxrWmpPacketSource.h"
#include "JxrLegacyBitIoBridge.h"

static Void JxrSharedBitReaderStateRefreshIfExternalStateChanged(JxrSharedBitReaderState* state)
{
    if (!JxrLegacyBitIoBridgeCursorMatches(state->legacyStream, &state->core.cursor) ||
        !JxrLegacyBitIoBridgeInputMatches(state->legacyStream, &state->core.input)) {
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
    return JxrBitReaderCorePeek(&state->core, count);
}

Void JxrSharedBitReaderStateConsume(JxrSharedBitReaderState* state, U32 count)
{
    JxrSharedBitReaderStateRefreshIfExternalStateChanged(state);
    JxrBitReaderCoreConsume(&state->core, count);
    JxrLegacyBitIoBridgeApplyCursor(state->legacyStream, &state->core.cursor);
}

U32 JxrSharedBitReaderStateReadLong(JxrSharedBitReaderState* state, U32 count)
{
    U32 value;
    JxrSharedBitReaderStateRefreshIfExternalStateChanged(state);
    value = JxrBitReaderCoreReadLong(&state->core, count);
    JxrLegacyBitIoBridgeApplyCursor(state->legacyStream, &state->core.cursor);
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
        size_t legacyStartIndex = (state->core.input.packetStartIndex + PACKETLENGTH) %
            state->core.input.length;
        size_t legacyOffset = before.offRef + PACKETLENGTH;

        JxrWmpPacketSourceInit(&wmpSource, before.pWS, &source);
        JxrBitReaderCoreTryRefill(&state->core, &source, PACKETLENGTH, &executorDidRefill);
        JxrLegacyBitIoBridgeApplyInput(state->legacyStream, &state->core.input);
        executorMatchesLegacy = executorDidRefill && legacyNeedsRefill &&
            state->core.input.packetStartIndex == legacyStartIndex &&
            state->core.input.currentIndex ==
                (size_t)(before.pbCurrent - state->core.input.buffer) &&
            state->core.input.streamOffset == legacyOffset &&
            state->core.input.shadow == *(U32*)before.pbStart;
        JXRTraceDumpRefillSnapshot(&before, state->legacyStream, TRUE, legacyNeedsRefill,
            executorDidRefill, executorMatchesLegacy,
            (UINTPTR_T)(state->core.input.buffer + state->core.input.packetStartIndex),
            (UINTPTR_T)(state->core.input.buffer + state->core.input.currentIndex),
            state->core.input.streamOffset, state->core.input.shadow,
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
    JxrLegacyBitIoBridgeRead(state->legacyStream, &state->core.cursor, &state->core.input);
    state->core.hasError = FALSE;
}

Bool JxrSharedBitReaderStateIsCurrent(const JxrSharedBitReaderState* state)
{
    return JxrLegacyBitIoBridgeCursorMatches(state->legacyStream, &state->core.cursor) &&
        JxrLegacyBitIoBridgeInputMatches(state->legacyStream, &state->core.input);
}

Bool JxrSharedBitReaderStateNeedsRefill(const JxrSharedBitReaderState* state)
{
    return JxrBitInputBufferStateNeedsRefill(&state->core.input, PACKETLENGTH);
}

Bool JxrSharedBitReaderStateSharesStream(const JxrSharedBitReaderState* left,
    const JxrSharedBitReaderState* right)
{
    return left->legacyStream == right->legacyStream;
}
