#include "JxrPacketExecutor.h"

Bool JxrPacketExecutorTryRefill(JxrBitInputBufferState* state, JxrPacketSource* source,
    U32 packetLength, Bool* didRefill, JxrPacketReadResult* result)
{
    *didRefill = FALSE;
    result->status = JxrPacketReadCompleted;
    result->bytesRead = 0;
    result->nativeError = WMP_errSuccess;
    if (!JxrBitInputBufferStateNeedsRefill(state, packetLength))
        return TRUE;
    if (!JxrBitInputBufferStateReadPacket(state, source, packetLength, result))
        return FALSE;
    *didRefill = TRUE;
    return TRUE;
}
