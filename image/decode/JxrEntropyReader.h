#ifndef JXR_ENTROPY_READER_H
#define JXR_ENTROPY_READER_H

#include "strcodec.h"

/* Explicit decoder-facing reader API; BitIOInfo remains inside the adapter. */
typedef struct JxrEntropyBitReader {
    BitIOInfo* legacyStream;
} JxrEntropyBitReader;

Void JxrEntropyBitReaderInit(JxrEntropyBitReader* state, BitIOInfo* legacyStream);
U32 JxrEntropyBitReaderPeek(JxrEntropyBitReader* state, U32 count);
Void JxrEntropyBitReaderConsume(JxrEntropyBitReader* state, U32 count);
U32 JxrEntropyBitReaderRead(JxrEntropyBitReader* state, U32 count);
U32 JxrEntropyBitReaderReadLong(JxrEntropyBitReader* state, U32 count);
U32 JxrEntropyBitReaderReadFlag(JxrEntropyBitReader* state);
I32 JxrEntropyBitReaderReadSign(JxrEntropyBitReader* state);
I32 JxrEntropyBitReaderDecodeSignedResidualValue(U32 encodedValue);
I32 JxrEntropyBitReaderReadSignedResidual(JxrEntropyBitReader* state, U32 count);

/* Compatibility wrappers for decoder code not yet migrated to the reader object. */
U32 JxrEntropyReaderPeek(BitIOInfo* state, U32 count);
Void JxrEntropyReaderConsume(BitIOInfo* state, U32 count);
U32 JxrEntropyReaderRead(BitIOInfo* state, U32 count);
U32 JxrEntropyReaderReadFlag(BitIOInfo* state);
I32 JxrEntropyReaderReadSign(BitIOInfo* state);
I32 JxrEntropyReaderReadSignedResidual(BitIOInfo* state, U32 count);

#endif
