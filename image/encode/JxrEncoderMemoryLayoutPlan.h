#ifndef JXR_ENCODER_MEMORY_LAYOUT_PLAN_H
#define JXR_ENCODER_MEMORY_LAYOUT_PLAN_H

#include "strcodec.h"

typedef struct JxrEncoderMemoryLayoutPlan {
    Bool allocationIsSafe;
    size_t macroblockCount;
    size_t fullResolutionMacroblockBytes;
    size_t chromaMacroblockBytes;
    size_t primaryMacroblockRowBytes;
    size_t primaryMacroblockBufferBytes;
    size_t primaryAllocationBytes;
    size_t secondaryMacroblockBufferBytes;
    size_t secondaryAllocationBytes;
} JxrEncoderMemoryLayoutPlan;

Void JxrEncoderMemoryLayoutPlanInitialize(JxrEncoderMemoryLayoutPlan* plan,
    size_t channelBytes, size_t chromaBlockCount, size_t channelCount,
    size_t imageWidth, size_t codecStateBytes, size_t bitIoStateBytes,
    Bool isThirtyTwoBitBuild);

#endif
