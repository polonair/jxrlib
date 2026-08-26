#ifndef JXR_DECODER_MEMORY_LAYOUT_PLAN_H
#define JXR_DECODER_MEMORY_LAYOUT_PLAN_H

#include "strcodec.h"

/*
 * Calculates the single primary decoder allocation before it is laid out by
 * ImageStrDecInit.  The plan deliberately contains sizes only: address
 * alignment remains the responsibility of the allocation step.
 */
typedef struct JxrDecoderMemoryLayoutPlan {
    Bool allocationIsSafe;
    size_t channelBytes;
    size_t chromaBlockCount;
    size_t macroblockCount;
    size_t fullResolutionMacroblockBytes;
    size_t chromaMacroblockBytes;
    size_t primaryMacroblockRowBytes;
    size_t primaryMacroblockBufferBytes;
    size_t allocationBytes;
} JxrDecoderMemoryLayoutPlan;

Void JxrDecoderMemoryLayoutPlanInitialize(JxrDecoderMemoryLayoutPlan* plan,
    BITDEPTH bitDepth, COLORFORMAT colorFormat, size_t channelCount,
    size_t imageWidth, size_t codecStateBytes, size_t decoderParametersBytes,
    size_t bitIoStateBytes, Bool isThirtyTwoBitBuild);

#endif
