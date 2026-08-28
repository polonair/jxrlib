#ifndef JXR_DECODER_NATIVE_OVERRIDE_PLAN_H
#define JXR_DECODER_NATIVE_OVERRIDE_PLAN_H

#include "JxrDecoderOptimizationPolicy.h"

typedef struct JxrDecoderNativeOverridePlan {
    Bool useRgb24Output;
    Bool useLossyRgb24Output;
    Bool useYuv444CenterTransform;
} JxrDecoderNativeOverridePlan;

Void JxrDecoderNativeOverridePlanInitialize(JxrDecoderNativeOverridePlan* plan,
    const CWMImageStrCodec* codec, const JxrDecoderOptimizationPolicy* policy);

#endif
