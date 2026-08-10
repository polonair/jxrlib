#ifndef JXR_BIT_INPUT_BUFFER_STATE_H
#define JXR_BIT_INPUT_BUFFER_STATE_H

#include "strcodec.h"

/* Value-based ring-buffer address calculations; no stream I/O yet. */
typedef struct JxrBitInputBufferState {
    UINTPTR_T startAddress;
    UINTPTR_T currentAddress;
    UINTPTR_T mask;
    size_t streamOffset;
    U32 shadow;
} JxrBitInputBufferState;

Void JxrBitInputBufferStateInit(JxrBitInputBufferState* state, UINTPTR_T startAddress,
    UINTPTR_T currentAddress, UINTPTR_T mask, size_t streamOffset, U32 shadow);
Bool JxrBitInputBufferStateNeedsRefill(const JxrBitInputBufferState* state, U32 packetLength);
Void JxrBitInputBufferStateAdvancePacketStart(JxrBitInputBufferState* state, U32 packetLength);

#endif
