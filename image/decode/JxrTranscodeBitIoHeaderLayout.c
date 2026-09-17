#include "JxrTranscodeBitIoHeaderLayout.h"

static Bool JxrTranscodeBitIoHeaderLayoutAdd(size_t left, size_t right,
    size_t* result)
{
    if (result == NULL || left > ((size_t)-1) - right) return FALSE;
    *result = left + right;
    return TRUE;
}

Bool JxrTranscodeBitIoHeaderLayoutInitialize(
    JxrTranscodeBitIoHeaderLayout* layout, UINTPTR_T allocationAddress,
    size_t bitIoStateBytes)
{
    const size_t packetAlignment = PACKETLENGTH * 4;
    size_t addressRemainder;
    size_t alignmentPadding;
    size_t allocationBytes;

    if (layout == NULL || packetAlignment == 0 || bitIoStateBytes == 0)
        return FALSE;
    addressRemainder = (size_t)(allocationAddress % packetAlignment);
    alignmentPadding = addressRemainder == 0 ? 0 : packetAlignment - addressRemainder;
    if (!JxrTranscodeBitIoHeaderLayoutAdd(alignmentPadding, PACKETLENGTH * 2,
        &layout->headerBitIoOffset) ||
        !JxrTranscodeBitIoHeaderLayoutAdd(packetAlignment - 1, packetAlignment,
            &allocationBytes) ||
        !JxrTranscodeBitIoHeaderLayoutAdd(allocationBytes, bitIoStateBytes,
            &layout->allocationBytes))
        return FALSE;
    return TRUE;
}

Bool JxrTranscodeBitIoHeaderLayoutBindCompatibilityPointer(
    CWMImageStrCodec* codec, U8* allocation,
    const JxrTranscodeBitIoHeaderLayout* layout)
{
    if (codec == NULL || allocation == NULL || layout == NULL ||
        layout->headerBitIoOffset > layout->allocationBytes ||
        sizeof(BitIOInfo) > layout->allocationBytes - layout->headerBitIoOffset)
        return FALSE;
    codec->pIOHeader = (BitIOInfo*)(allocation + layout->headerBitIoOffset);
    return TRUE;
}
