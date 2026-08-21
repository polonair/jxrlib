#ifndef JXR_ENCODER_CHROMA_RESAMPLING_SETUP_H
#define JXR_ENCODER_CHROMA_RESAMPLING_SETUP_H

#include "strcodec.h"

typedef struct JxrEncoderChromaResamplingPlan {
    Bool changesUvResolution;
    Bool allocationIsSafe;
    size_t residualRowStride;
    size_t residualSampleCount;
} JxrEncoderChromaResamplingPlan;

Void JxrEncoderChromaResamplingPlanInitialize(JxrEncoderChromaResamplingPlan* plan,
    COLORFORMAT sourceFormat, COLORFORMAT targetFormat, Bool inputIsYuvData,
    size_t macroblockWidth, Bool isThirtyTwoBitBuild);
Int JxrEncoderChromaResamplingSetupInitialize(CWMImageStrCodec* codec);

#endif
