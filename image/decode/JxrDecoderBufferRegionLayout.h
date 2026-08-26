#ifndef JXR_DECODER_BUFFER_REGION_LAYOUT_H
#define JXR_DECODER_BUFFER_REGION_LAYOUT_H

#include "JxrDecoderMemoryLayoutPlan.h"

/* Offsets within the single allocation used by the primary decoder plane. */
typedef struct JxrDecoderBufferRegionLayout {
    size_t decoderParametersOffset;
    size_t macroblockBufferOffset;
    size_t headerBitIoOffset;
    size_t allocationUsedBytes;
} JxrDecoderBufferRegionLayout;

Void JxrDecoderBufferRegionLayoutInitialize(JxrDecoderBufferRegionLayout* layout,
    UINTPTR_T allocationAddress, size_t codecStateBytes,
    size_t decoderParametersBytes, size_t bitIoStateBytes,
    const JxrDecoderMemoryLayoutPlan* memoryLayout);
Void JxrDecoderBufferRegionLayoutBind(CWMImageStrCodec* codec, U8* allocation,
    const JxrDecoderBufferRegionLayout* layout,
    size_t fullResolutionMacroblockBytes, size_t chromaMacroblockBytes);

#endif
