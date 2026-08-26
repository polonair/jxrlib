#include "JxrDecoderBufferRegionLayout.h"
#include "decode.h"

static UINTPTR_T JxrDecoderBufferRegionLayoutAlign(UINTPTR_T address, size_t alignment)
{
    return (address + alignment - 1) & ~(alignment - 1);
}

Void JxrDecoderBufferRegionLayoutInitialize(JxrDecoderBufferRegionLayout* layout,
    UINTPTR_T allocationAddress, size_t codecStateBytes,
    size_t decoderParametersBytes, size_t bitIoStateBytes,
    const JxrDecoderMemoryLayoutPlan* memoryLayout)
{
    UINTPTR_T macroblockBufferAddress;
    UINTPTR_T headerBitIoAddress;

    layout->decoderParametersOffset = codecStateBytes;
    macroblockBufferAddress = JxrDecoderBufferRegionLayoutAlign(
        allocationAddress + codecStateBytes + decoderParametersBytes, 128);
    headerBitIoAddress = JxrDecoderBufferRegionLayoutAlign(
        macroblockBufferAddress + memoryLayout->primaryMacroblockBufferBytes,
        PACKETLENGTH * 4) + PACKETLENGTH * 2;

    layout->macroblockBufferOffset = (size_t)(macroblockBufferAddress - allocationAddress);
    layout->headerBitIoOffset = (size_t)(headerBitIoAddress - allocationAddress);
    layout->allocationUsedBytes = layout->headerBitIoOffset + bitIoStateBytes;
}

Void JxrDecoderBufferRegionLayoutBind(CWMImageStrCodec* codec, U8* allocation,
    const JxrDecoderBufferRegionLayout* layout,
    size_t fullResolutionMacroblockBytes, size_t chromaMacroblockBytes)
{
    U8* buffer = allocation + layout->macroblockBufferOffset;
    size_t channel;
    size_t macroblockStride = fullResolutionMacroblockBytes;

    codec->m_Dparam = (CWMDecoderParameters*)(allocation +
        layout->decoderParametersOffset);
    for (channel = 0; channel < codec->m_param.cNumChannels; ++channel) {
        codec->a0MBbuffer[channel] = (PixelI*)buffer;
        buffer += macroblockStride * codec->cmbWidth;
        codec->a1MBbuffer[channel] = (PixelI*)buffer;
        buffer += macroblockStride * codec->cmbWidth;
        macroblockStride = chromaMacroblockBytes;
    }
    codec->pIOHeader = (BitIOInfo*)(allocation + layout->headerBitIoOffset);
}
