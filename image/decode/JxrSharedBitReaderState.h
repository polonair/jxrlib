#ifndef JXR_SHARED_BIT_READER_STATE_H
#define JXR_SHARED_BIT_READER_STATE_H

#include "strcodec.h"
#include "JxrBitReaderCore.h"

/* Shared transport state for all entropy readers that alias one BitIOInfo. */
typedef struct JxrSharedBitReaderState {
    BitIOInfo* legacyStream;
    JxrBitReaderCore core;
} JxrSharedBitReaderState;

Void JxrSharedBitReaderStateInit(JxrSharedBitReaderState* state, BitIOInfo* legacyStream);
U32 JxrSharedBitReaderStatePeek(JxrSharedBitReaderState* state, U32 count);
Void JxrSharedBitReaderStateConsume(JxrSharedBitReaderState* state, U32 count);
U32 JxrSharedBitReaderStateReadLong(JxrSharedBitReaderState* state, U32 count);
Bool JxrSharedBitReaderStateRefillLevel1(CWMImageStrCodec* codec, JxrSharedBitReaderState* state);
Bool JxrSharedBitReaderStateRefillLevel2(CWMImageStrCodec* codec, JxrSharedBitReaderState* state);
Void JxrSharedBitReaderStateSyncFromLegacy(JxrSharedBitReaderState* state);
Bool JxrSharedBitReaderStateIsCurrent(const JxrSharedBitReaderState* state);
Bool JxrSharedBitReaderStateNeedsRefill(const JxrSharedBitReaderState* state);
Bool JxrSharedBitReaderStateSharesStream(const JxrSharedBitReaderState* left,
    const JxrSharedBitReaderState* right);

#endif
