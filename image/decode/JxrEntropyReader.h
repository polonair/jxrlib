#ifndef JXR_ENTROPY_READER_H
#define JXR_ENTROPY_READER_H

#include "strcodec.h"

/* Explicit decoder-facing reader API; BitIOInfo remains the stream adapter. */
U32 JxrEntropyReaderPeek(BitIOInfo* state, U32 count);
Void JxrEntropyReaderConsume(BitIOInfo* state, U32 count);
U32 JxrEntropyReaderRead(BitIOInfo* state, U32 count);
U32 JxrEntropyReaderReadFlag(BitIOInfo* state);
I32 JxrEntropyReaderReadSign(BitIOInfo* state);
I32 JxrEntropyReaderReadSignedResidual(BitIOInfo* state, U32 count);

#endif
