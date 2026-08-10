#include "JxrTranscodeQuantizerWriter.h"

static Bool JxrTranscodeBitSinkWriteLegacy(Void* context, U32 value, U32 count)
{
    putBit16((BitIOInfo*)context, value, count);
    return TRUE;
}

Bool JxrTranscodeBitSinkWriteBits(JxrTranscodeBitSink* sink, U32 value, U32 count)
{
    return sink != NULL && sink->write != NULL && sink->write(sink->context, value, count);
}

Void JxrTranscodeBitSinkInit(JxrTranscodeBitSink* sink, Void* context,
    JxrTranscodeBitSinkWrite write)
{
    sink->context = context;
    sink->write = write;
}

Void JxrTranscodeBitSinkInitLegacy(JxrTranscodeBitSink* sink, BitIOInfo* nativeOutput)
{
    JxrTranscodeBitSinkInit(sink, nativeOutput, JxrTranscodeBitSinkWriteLegacy);
}

Bool JxrTranscodeQuantizerWriterWriteQuantizer(JxrTranscodeBitSink* sink,
    const U8 indices[MAX_CHANNELS], U8 channelMode, size_t channelCount)
{
    size_t channel;

    if (indices == NULL || channelCount == 0 || channelCount > MAX_CHANNELS) return FALSE;
    if (channelMode > 2) channelMode = 2;
    if (channelCount > 1) {
        if (!JxrTranscodeBitSinkWriteBits(sink, channelMode, 2)) return FALSE;
    }
    else channelMode = 0;

    if (!JxrTranscodeBitSinkWriteBits(sink, indices[0], 8)) return FALSE;
    if (channelMode == 1)
        return JxrTranscodeBitSinkWriteBits(sink, indices[1], 8);
    if (channelMode > 0)
        for (channel = 1; channel < channelCount; ++channel)
            if (!JxrTranscodeBitSinkWriteBits(sink, indices[channel], 8)) return FALSE;
    return TRUE;
}

Bool JxrTranscodeQuantizerWriterWriteQuantizers(JxrTranscodeBitSink* sink,
    const U8 indices[JXR_TRANSCODE_MAX_QUANTIZERS][MAX_CHANNELS],
    const U8 channelModes[JXR_TRANSCODE_MAX_QUANTIZERS], U32 quantizerCount,
    size_t channelCount, Bool copyPrevious)
{
    U32 quantizer;

    if (indices == NULL || channelModes == NULL || quantizerCount == 0 ||
        quantizerCount > JXR_TRANSCODE_MAX_QUANTIZERS) return FALSE;
    if (!JxrTranscodeBitSinkWriteBits(sink, copyPrevious == TRUE ? 1 : 0, 1)) return FALSE;
    if (copyPrevious == TRUE) return TRUE;
    if (!JxrTranscodeBitSinkWriteBits(sink, quantizerCount - 1, 4)) return FALSE;
    for (quantizer = 0; quantizer < quantizerCount; ++quantizer)
        if (!JxrTranscodeQuantizerWriterWriteQuantizer(sink, indices[quantizer],
            channelModes[quantizer], channelCount)) return FALSE;
    return TRUE;
}

Bool JxrTranscodeQuantizerWriterWriteAlphaQuantizers(JxrTranscodeBitSink* sink,
    const U8 indices[JXR_TRANSCODE_MAX_QUANTIZERS][MAX_CHANNELS], U32 quantizerCount,
    size_t alphaChannelIndex, Bool copyPrevious)
{
    U32 quantizer;

    if (indices == NULL || alphaChannelIndex >= MAX_CHANNELS || quantizerCount == 0 ||
        quantizerCount > JXR_TRANSCODE_MAX_QUANTIZERS) return FALSE;
    if (!JxrTranscodeBitSinkWriteBits(sink, copyPrevious == TRUE ? 1 : 0, 1)) return FALSE;
    if (copyPrevious == TRUE) return TRUE;
    if (!JxrTranscodeBitSinkWriteBits(sink, quantizerCount - 1, 4)) return FALSE;
    for (quantizer = 0; quantizer < quantizerCount; ++quantizer)
        if (!JxrTranscodeBitSinkWriteBits(sink, indices[quantizer][alphaChannelIndex], 8)) return FALSE;
    return TRUE;
}
