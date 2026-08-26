#include "JxrSecondaryPlaneBufferRegionLayout.h"

static UINTPTR_T JxrSecondaryPlaneBufferRegionLayoutAlign(
    UINTPTR_T address, size_t alignment)
{
    return (address + alignment - 1) & ~(alignment - 1);
}

Void JxrSecondaryPlaneBufferRegionLayoutInitialize(
    JxrSecondaryPlaneBufferRegionLayout* layout, UINTPTR_T allocationAddress,
    size_t codecStateBytes, const JxrSecondaryPlaneMemoryLayoutPlan* memoryLayout)
{
    UINTPTR_T macroblockBufferAddress = JxrSecondaryPlaneBufferRegionLayoutAlign(
        allocationAddress + codecStateBytes, 128);

    layout->macroblockBufferOffset =
        (size_t)(macroblockBufferAddress - allocationAddress);
    layout->allocationUsedBytes = layout->macroblockBufferOffset +
        memoryLayout->macroblockBufferBytes;
}

Void JxrSecondaryPlaneBufferRegionLayoutBind(CWMImageStrCodec* codec, U8* allocation,
    const JxrSecondaryPlaneBufferRegionLayout* layout, size_t macroblockStride)
{
    U8* buffer = allocation + layout->macroblockBufferOffset;

    codec->a0MBbuffer[0] = (PixelI*)buffer;
    buffer += macroblockStride * codec->cmbWidth;
    codec->a1MBbuffer[0] = (PixelI*)buffer;
}
