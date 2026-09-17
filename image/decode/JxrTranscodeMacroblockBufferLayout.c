#include "JxrTranscodeMacroblockBufferLayout.h"

static Bool JxrTranscodeMacroblockBufferLayoutCalculateCoefficientCount(
    COLORFORMAT colorFormat, size_t channelCount, size_t* coefficientCount)
{
    if (coefficientCount == NULL || channelCount == 0 || channelCount > MAX_CHANNELS)
        return FALSE;
    if (colorFormat == YUV_420) {
        *coefficientCount = 384;
        return TRUE;
    }
    if (colorFormat == YUV_422) {
        *coefficientCount = 512;
        return TRUE;
    }
    if (channelCount > ((size_t)-1) / 256) return FALSE;
    *coefficientCount = 256 * channelCount;
    return TRUE;
}

Bool JxrTranscodeMacroblockBufferLayoutInitialize(COLORFORMAT colorFormat,
    size_t channelCount, JxrTranscodeMacroblockBufferLayout* layout)
{
    size_t channel;
    size_t channelStride;

    if (layout == NULL || !JxrTranscodeMacroblockBufferLayoutCalculateCoefficientCount(
        colorFormat, channelCount, &layout->coefficientCount))
        return FALSE;
    memset(layout->channelOffsets, 0, sizeof(layout->channelOffsets));
    layout->channelCount = channelCount;
    if (channelCount == 1) return TRUE;

    layout->channelOffsets[1] = 256;
    channelStride = colorFormat == YUV_420 ? 64 :
        (colorFormat == YUV_422 ? 128 : 256);
    for (channel = 2; channel < channelCount; channel++)
        layout->channelOffsets[channel] = layout->channelOffsets[channel - 1] +
            channelStride;
    return TRUE;
}

Bool JxrTranscodeMacroblockBufferLayoutBindCompatibilityPointers(
    CWMImageStrCodec* codec, PixelI* coefficientBuffer,
    const JxrTranscodeMacroblockBufferLayout* layout)
{
    size_t channel;

    if (codec == NULL || coefficientBuffer == NULL || layout == NULL ||
        layout->channelCount == 0 || layout->channelCount > MAX_CHANNELS)
        return FALSE;
    for (channel = 0; channel < layout->channelCount; channel++)
        codec->p1MBbuffer[channel] = coefficientBuffer + layout->channelOffsets[channel];
    /* Preserve the legacy one-past-the-plane pointer for single-channel codecs. */
    if (layout->channelCount == 1)
        codec->p1MBbuffer[1] = coefficientBuffer + 256;
    return TRUE;
}
