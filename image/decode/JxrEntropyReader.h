#ifndef JXR_ENTROPY_READER_H
#define JXR_ENTROPY_READER_H

#include "JxrSharedBitReaderState.h"

/* Explicit reader view: position/error are per caller; transport state may be shared. */
typedef struct JxrEntropyBitReader {
    JxrSharedBitReaderState ownedState;
    JxrSharedBitReaderState* sharedState;
    U32 positionBits;
    Bool hasError;
} JxrEntropyBitReader;

Void JxrEntropyBitReaderInit(JxrEntropyBitReader* state, BitIOInfo* legacyStream);
Void JxrEntropyBitReaderInitShared(JxrEntropyBitReader* state, JxrSharedBitReaderState* sharedState);
U32 JxrEntropyBitReaderPeek(JxrEntropyBitReader* state, U32 count);
Void JxrEntropyBitReaderConsume(JxrEntropyBitReader* state, U32 count);
U32 JxrEntropyBitReaderRead(JxrEntropyBitReader* state, U32 count);
U32 JxrEntropyBitReaderReadLong(JxrEntropyBitReader* state, U32 count);
U32 JxrEntropyBitReaderReadFlag(JxrEntropyBitReader* state);
I32 JxrEntropyBitReaderReadSign(JxrEntropyBitReader* state);
I32 JxrEntropyBitReaderDecodeSignedResidualValue(U32 encodedValue);
I32 JxrEntropyBitReaderReadSignedResidual(JxrEntropyBitReader* state, U32 count);
U32 JxrEntropyBitReaderPosition(const JxrEntropyBitReader* state);
Bool JxrEntropyBitReaderHasError(const JxrEntropyBitReader* state);
Bool JxrEntropyBitReaderSharesStream(const JxrEntropyBitReader* left,
    const JxrEntropyBitReader* right);

/* Compatibility wrappers for decoder code not yet migrated to the reader object. */
U32 JxrEntropyReaderPeek(BitIOInfo* state, U32 count);
Void JxrEntropyReaderConsume(BitIOInfo* state, U32 count);
U32 JxrEntropyReaderRead(BitIOInfo* state, U32 count);
U32 JxrEntropyReaderReadFlag(BitIOInfo* state);
I32 JxrEntropyReaderReadSign(BitIOInfo* state);
I32 JxrEntropyReaderReadSignedResidual(BitIOInfo* state, U32 count);

#endif
