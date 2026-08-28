#ifndef JXR_DECODER_OPTIMIZATION_POLICY_H
#define JXR_DECODER_OPTIMIZATION_POLICY_H

#include "strcodec.h"

/* Separates optional native fast paths from the portable decoder baseline. */
typedef struct JxrDecoderOptimizationPolicy {
    Bool nativeOptimizationAvailable;
    Bool usesPortableBaseline;
} JxrDecoderOptimizationPolicy;

Void JxrDecoderOptimizationPolicyInitialize(JxrDecoderOptimizationPolicy* policy);

#endif
