#ifndef JXR_ENCODER_CHROMA_DOWNSAMPLER_H
#define JXR_ENCODER_CHROMA_DOWNSAMPLER_H

#include "strcodec.h"

typedef struct JxrEncoderChromaDownsamplingPlan {
    Bool performsHorizontalDownsampling;
    Bool writesHorizontalResultToMacroblockBuffer;
    Bool performsVerticalDownsampling;
} JxrEncoderChromaDownsamplingPlan;

Void JxrEncoderChromaDownsamplingPlanInitialize(JxrEncoderChromaDownsamplingPlan* plan,
    COLORFORMAT sourceFormat, COLORFORMAT targetFormat);
PixelI JxrEncoderChromaDownsamplerFilterOdd(PixelI first, PixelI second,
    PixelI center, PixelI fourth, PixelI fifth);
Void JxrEncoderChromaDownsamplerApply(CWMImageStrCodec* codec);

#endif
