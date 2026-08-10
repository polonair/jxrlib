#ifndef JXR_PACKET_EXECUTOR_H
#define JXR_PACKET_EXECUTOR_H

#include "JxrBitInputBufferState.h"

/* Managed-style packet refill: explicit state, source and destination. */
Bool JxrPacketExecutorTryRefill(JxrBitInputBufferState* state, JxrPacketSource* source,
    U8* destination, U32 packetLength, Bool* didRefill);

#endif
