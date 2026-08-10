#ifndef JXR_BIT_READER_CORE_H
#define JXR_BIT_READER_CORE_H

#include "JxrBitCursorState.h"
#include "JxrBitInputBufferState.h"
#include "JxrPacketSource.h"

/* Managed-port candidate: no BitIOInfo, WMPStream or codec dependencies. */
typedef struct JxrBitReaderCore {
    JxrBitCursorState cursor;
    JxrBitInputBufferState input;
    Bool hasError;
} JxrBitReaderCore;

Void JxrBitReaderCoreInit(JxrBitReaderCore* state, U8* buffer, size_t length,
    size_t packetStartIndex, size_t currentIndex, size_t streamOffset,
    U32 shadow, U32 accumulator, U32 usedBits);
U32 JxrBitReaderCorePeek(JxrBitReaderCore* state, U32 count);
Void JxrBitReaderCoreConsume(JxrBitReaderCore* state, U32 count);
U32 JxrBitReaderCoreReadLong(JxrBitReaderCore* state, U32 count);
Bool JxrBitReaderCoreTryRefill(JxrBitReaderCore* state, JxrPacketSource* source,
    U32 packetLength, Bool* didRefill);

#endif
