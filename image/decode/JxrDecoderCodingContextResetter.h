#ifndef JXR_DECODER_CODING_CONTEXT_RESETTER_H
#define JXR_DECODER_CODING_CONTEXT_RESETTER_H

#include "strcodec.h"

typedef struct JxrDecoderCodingContextResetterConfig {
    CCodingContext* contexts;
    size_t contextCount;
    Bool resetAll;
} JxrDecoderCodingContextResetterConfig;

typedef Void (*JxrDecoderCodingContextResetterReset)(Void* context,
    CCodingContext* codingContext);
typedef struct JxrDecoderCodingContextResetterOperations {
    Void* context;
    JxrDecoderCodingContextResetterReset reset;
} JxrDecoderCodingContextResetterOperations;

/* Resets the first context or every context in the supplied tile row. */
Bool JxrDecoderCodingContextResetterResetContexts(
    const JxrDecoderCodingContextResetterConfig* config,
    const JxrDecoderCodingContextResetterOperations* operations);

#endif
