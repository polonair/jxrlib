#ifndef JXR_BIT_INPUT_BUFFER_STATE_H
#define JXR_BIT_INPUT_BUFFER_STATE_H

#include "windowsmediaphoto.h"
#include "JxrPacketSource.h"

/* Managed-style packet state: byte buffer plus explicit ring indices. */
typedef struct JxrBitInputBufferState {
    U8* buffer;
    size_t length;
    size_t packetStartIndex;
    size_t currentIndex;
    size_t streamOffset;
    U32 shadow;
} JxrBitInputBufferState;

Void JxrBitInputBufferStateInit(JxrBitInputBufferState* state, U8* buffer, size_t length,
    size_t packetStartIndex, size_t currentIndex, size_t streamOffset, U32 shadow);
Bool JxrBitInputBufferStateNeedsRefill(const JxrBitInputBufferState* state, U32 packetLength);
Void JxrBitInputBufferStateAdvancePacketStart(JxrBitInputBufferState* state, U32 packetLength);
Bool JxrBitInputBufferStateReadPacket(JxrBitInputBufferState* state, JxrPacketSource* source,
    U32 packetLength);

#endif
