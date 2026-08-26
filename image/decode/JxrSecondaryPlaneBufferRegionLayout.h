#ifndef JXR_SECONDARY_PLANE_BUFFER_REGION_LAYOUT_H
#define JXR_SECONDARY_PLANE_BUFFER_REGION_LAYOUT_H

#include "JxrSecondaryPlaneMemoryLayoutPlan.h"

typedef struct JxrSecondaryPlaneBufferRegionLayout {
    size_t macroblockBufferOffset;
    size_t allocationUsedBytes;
} JxrSecondaryPlaneBufferRegionLayout;

Void JxrSecondaryPlaneBufferRegionLayoutInitialize(
    JxrSecondaryPlaneBufferRegionLayout* layout, UINTPTR_T allocationAddress,
    size_t codecStateBytes, const JxrSecondaryPlaneMemoryLayoutPlan* memoryLayout);
Void JxrSecondaryPlaneBufferRegionLayoutBind(CWMImageStrCodec* codec, U8* allocation,
    const JxrSecondaryPlaneBufferRegionLayout* layout, size_t macroblockStride);

#endif
