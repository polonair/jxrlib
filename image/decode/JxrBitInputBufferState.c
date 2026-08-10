#include "JxrBitInputBufferState.h"

Void JxrBitInputBufferStateInit(JxrBitInputBufferState* state, UINTPTR_T startAddress,
    UINTPTR_T currentAddress, UINTPTR_T mask, size_t streamOffset, U32 shadow)
{
    state->startAddress = startAddress;
    state->currentAddress = currentAddress;
    state->mask = mask;
    state->streamOffset = streamOffset;
    state->shadow = shadow;
}

Bool JxrBitInputBufferStateNeedsRefill(const JxrBitInputBufferState* state, U32 packetLength)
{
    return ((state->startAddress ^ state->currentAddress) & packetLength) != 0;
}

Void JxrBitInputBufferStateAdvancePacketStart(JxrBitInputBufferState* state, U32 packetLength)
{
    state->startAddress = (state->startAddress + packetLength) & state->mask;
}

Bool JxrBitInputBufferStateReadPacket(JxrBitInputBufferState* state, JxrPacketSource* source,
    U8* destination, U32 packetLength)
{
    U32 shadow;
    if (!JxrPacketSourceRead(source, state->streamOffset, destination, packetLength)) return FALSE;
    memcpy(&shadow, destination, sizeof(shadow));
    state->shadow = shadow;
    state->streamOffset += packetLength;
    JxrBitInputBufferStateAdvancePacketStart(state, packetLength);
    return TRUE;
}
