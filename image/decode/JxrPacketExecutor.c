#include "JxrPacketExecutor.h"

Bool JxrPacketExecutorTryRefill(JxrBitInputBufferState* state, JxrPacketSource* source,
    U8* destination, U32 packetLength, Bool* didRefill)
{
    *didRefill = FALSE;
    if (!JxrBitInputBufferStateNeedsRefill(state, packetLength))
        return TRUE;
    if (!JxrBitInputBufferStateReadPacket(state, source, destination, packetLength))
        return FALSE;
    *didRefill = TRUE;
    return TRUE;
}
