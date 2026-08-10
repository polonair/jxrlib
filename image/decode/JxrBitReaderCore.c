#include "JxrBitReaderCore.h"
#include "JxrPacketExecutor.h"

Void JxrBitReaderCoreInit(JxrBitReaderCore* state, U8* buffer, size_t length,
    size_t packetStartIndex, size_t currentIndex, size_t streamOffset,
    U32 shadow, U32 accumulator, U32 usedBits)
{
    JxrBitCursorStateInit(&state->cursor, buffer, length, currentIndex, accumulator, usedBits);
    JxrBitInputBufferStateInit(&state->input, buffer, length, packetStartIndex,
        currentIndex, streamOffset, shadow);
    state->hasError = FALSE;
    state->lastPacketRead.status = JxrPacketReadCompleted;
    state->lastPacketRead.bytesRead = 0;
    state->lastPacketRead.nativeError = WMP_errSuccess;
}

U32 JxrBitReaderCorePeek(JxrBitReaderCore* state, U32 count)
{
    return JxrBitCursorStatePeek(&state->cursor, count);
}

Void JxrBitReaderCoreConsume(JxrBitReaderCore* state, U32 count)
{
    JxrBitCursorStateConsume(&state->cursor, count);
    state->input.currentIndex = state->cursor.currentIndex;
}

U32 JxrBitReaderCoreReadLong(JxrBitReaderCore* state, U32 count)
{
    U32 value = JxrBitCursorStateReadLong(&state->cursor, count);
    state->input.currentIndex = state->cursor.currentIndex;
    return value;
}

Bool JxrBitReaderCoreTryRefill(JxrBitReaderCore* state, JxrPacketSource* source,
    U32 packetLength, Bool* didRefill)
{
    if (!JxrPacketExecutorTryRefill(&state->input, source, packetLength, didRefill,
        &state->lastPacketRead)) {
        state->hasError = TRUE;
        return FALSE;
    }
    return TRUE;
}
