#include "JxrPacketExecutor.h"

Bool JxrPacketExecutorTryRefill(JxrBitInputBufferState* state, JxrPacketSource* source,
    U32 packetLength, Bool* didRefill)
{
    *didRefill = FALSE;
    if (!JxrBitInputBufferStateNeedsRefill(state, packetLength))
        return TRUE;
    if (!JxrBitInputBufferStateReadPacket(state, source, packetLength))
        return FALSE;
    *didRefill = TRUE;
    return TRUE;
}
