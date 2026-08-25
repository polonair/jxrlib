#ifndef JXR_ENCODER_BUFFER_REGION_LAYOUT_H
#define JXR_ENCODER_BUFFER_REGION_LAYOUT_H

#include "JxrEncoderMemoryLayoutPlan.h"

typedef struct JxrEncoderBufferRegionLayout {
    size_t macroblockBufferOffset;
    size_t headerBitIoOffset;
} JxrEncoderBufferRegionLayout;

Void JxrEncoderBufferRegionLayoutInitialize(JxrEncoderBufferRegionLayout* layout,
    UINTPTR_T allocationAddress, size_t codecStateBytes,
    const JxrEncoderMemoryLayoutPlan* memoryLayout);
Void JxrEncoderBufferRegionLayoutBindPrimary(CWMImageStrCodec* codec,
    U8* allocation, const JxrEncoderBufferRegionLayout* layout,
    size_t fullResolutionMacroblockBytes, size_t chromaMacroblockBytes);
Void JxrEncoderBufferRegionLayoutBindSecondary(CWMImageStrCodec* codec,
    U8* allocation, const JxrEncoderBufferRegionLayout* layout,
    size_t fullResolutionMacroblockBytes);

#endif
