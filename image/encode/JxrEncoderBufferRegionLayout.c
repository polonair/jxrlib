#include "JxrEncoderBufferRegionLayout.h"

static UINTPTR_T JxrEncoderBufferRegionLayoutAlign(UINTPTR_T address, size_t alignment)
{
    return (address + alignment - 1) & ~(alignment - 1);
}

Void JxrEncoderBufferRegionLayoutInitialize(JxrEncoderBufferRegionLayout* layout,
    UINTPTR_T allocationAddress, size_t codecStateBytes,
    const JxrEncoderMemoryLayoutPlan* memoryLayout)
{
    UINTPTR_T macroblockBufferAddress = JxrEncoderBufferRegionLayoutAlign(
        allocationAddress + codecStateBytes, 128);
    UINTPTR_T headerBitIoAddress = JxrEncoderBufferRegionLayoutAlign(
        macroblockBufferAddress + memoryLayout->primaryMacroblockBufferBytes,
        PACKETLENGTH * 4) + PACKETLENGTH * 2;

    layout->macroblockBufferOffset = (size_t)(macroblockBufferAddress - allocationAddress);
    layout->headerBitIoOffset = (size_t)(headerBitIoAddress - allocationAddress);
}

Void JxrEncoderBufferRegionLayoutBindPrimary(CWMImageStrCodec* codec,
    U8* allocation, const JxrEncoderBufferRegionLayout* layout,
    size_t fullResolutionMacroblockBytes, size_t chromaMacroblockBytes)
{
    U8* buffer = allocation + layout->macroblockBufferOffset;
    size_t channel;
    size_t macroblockStride = fullResolutionMacroblockBytes;

    for (channel = 0; channel < codec->m_param.cNumChannels; ++channel) {
        codec->a0MBbuffer[channel] = (PixelI*)buffer;
        buffer += macroblockStride * codec->cmbWidth;
        codec->a1MBbuffer[channel] = (PixelI*)buffer;
        buffer += macroblockStride * codec->cmbWidth;
        macroblockStride = chromaMacroblockBytes;
    }
    codec->pIOHeader = (BitIOInfo*)(allocation + layout->headerBitIoOffset);
}

Void JxrEncoderBufferRegionLayoutBindSecondary(CWMImageStrCodec* codec,
    U8* allocation, const JxrEncoderBufferRegionLayout* layout,
    size_t fullResolutionMacroblockBytes)
{
    U8* buffer = allocation + layout->macroblockBufferOffset;

    codec->a0MBbuffer[0] = (PixelI*)buffer;
    buffer += fullResolutionMacroblockBytes * codec->cmbWidth;
    codec->a1MBbuffer[0] = (PixelI*)buffer;
}
