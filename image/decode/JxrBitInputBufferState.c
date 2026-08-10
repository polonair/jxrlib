#include "JxrBitInputBufferState.h"

#include <assert.h>
#include <string.h>

Void JxrBitInputBufferStateInit(JxrBitInputBufferState* state, U8* buffer, size_t length,
    size_t packetStartIndex, size_t currentIndex, size_t streamOffset, U32 shadow)
{
    assert(buffer != NULL && length != 0 && packetStartIndex < length && currentIndex < length);
    state->buffer = buffer;
    state->length = length;
    state->packetStartIndex = packetStartIndex;
    state->currentIndex = currentIndex;
    state->streamOffset = streamOffset;
    state->shadow = shadow;
}

Bool JxrBitInputBufferStateNeedsRefill(const JxrBitInputBufferState* state, U32 packetLength)
{
    return ((state->packetStartIndex ^ state->currentIndex) & packetLength) != 0;
}

Void JxrBitInputBufferStateAdvancePacketStart(JxrBitInputBufferState* state, U32 packetLength)
{
    state->packetStartIndex = (state->packetStartIndex + packetLength) % state->length;
}

Bool JxrBitInputBufferStateReadPacket(JxrBitInputBufferState* state, JxrPacketSource* source,
    U32 packetLength, JxrPacketReadResult* result)
{
    U32 shadow;
    assert(state->packetStartIndex + packetLength <= state->length);
    *result = JxrPacketSourceRead(source, state->streamOffset,
        state->buffer + state->packetStartIndex, packetLength);
    if (result->status == JxrPacketReadFailed) return FALSE;
    memcpy(&shadow, state->buffer + state->packetStartIndex, sizeof(shadow));
    state->shadow = shadow;
    state->streamOffset += packetLength;
    JxrBitInputBufferStateAdvancePacketStart(state, packetLength);
    return TRUE;
}
