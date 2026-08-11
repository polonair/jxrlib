#include "JxrTranscodeTileHeaderWriter.h"

static Bool JxrTranscodeTileHeaderWriterWritePacket(JxrTranscodeBitSink* output,
    U8 packetType, U8 tileId)
{
    return JxrTranscodeBitSinkWriteBits(output, 0, 8) &&
        JxrTranscodeBitSinkWriteBits(output, 0, 8) &&
        JxrTranscodeBitSinkWriteBits(output, 1, 8) &&
        JxrTranscodeBitSinkWriteBits(output, (tileId << 3) + (packetType & 7), 8);
}

static Bool JxrTranscodeTileHeaderWriterWriteDc(const JxrTranscodeTileHeaderState* state,
    JxrTranscodeBitSink* output)
{
    const JxrTranscodeTileQuantizerState* quantizers = state->quantizers;

    if ((state->quantizerMode & 1) != 0 &&
        !JxrTranscodeQuantizerWriterWriteQuantizer(output, quantizers->dcIndex,
            quantizers->dcMode, state->channelCount)) return FALSE;
    if (state->hasAlpha && (state->quantizerMode & 1) != 0 &&
        !JxrTranscodeBitSinkWriteBits(output,
            quantizers->dcIndex[state->alphaChannelIndex], 8)) return FALSE;
    return TRUE;
}

static Bool JxrTranscodeTileHeaderWriterWriteLowpass(const JxrTranscodeTileHeaderState* state,
    JxrTranscodeBitSink* output)
{
    const JxrTranscodeTileQuantizerState* quantizers = state->quantizers;

    if ((state->quantizerMode & 2) != 0 &&
        !JxrTranscodeQuantizerWriterWriteQuantizers(output, quantizers->lowpassIndex,
            quantizers->lowpassMode, quantizers->lowpassQuantizerCount,
            state->channelCount, quantizers->useDcForLowpass)) return FALSE;
    if (state->hasAlpha && (state->quantizerMode & 2) != 0 &&
        !JxrTranscodeQuantizerWriterWriteAlphaQuantizers(output, quantizers->lowpassIndex,
            quantizers->lowpassQuantizerCountAlpha, state->alphaChannelIndex,
            quantizers->useDcForLowpassAlpha)) return FALSE;
    return TRUE;
}

static Bool JxrTranscodeTileHeaderWriterWriteHighpass(const JxrTranscodeTileHeaderState* state,
    JxrTranscodeBitSink* output)
{
    const JxrTranscodeTileQuantizerState* quantizers = state->quantizers;

    if ((state->quantizerMode & 4) != 0 &&
        !JxrTranscodeQuantizerWriterWriteQuantizers(output, quantizers->highpassIndex,
            quantizers->highpassMode, quantizers->highpassQuantizerCount,
            state->channelCount, quantizers->useLowpassForHighpass)) return FALSE;
    if (state->hasAlpha && (state->quantizerMode & 4) != 0 &&
        !JxrTranscodeQuantizerWriterWriteAlphaQuantizers(output, quantizers->highpassIndex,
            quantizers->highpassQuantizerCountAlpha, state->alphaChannelIndex,
            quantizers->useLowpassForHighpassAlpha)) return FALSE;
    return TRUE;
}

Bool JxrTranscodeTileHeaderWriterWrite(const JxrTranscodeTileHeaderState* state,
    JxrTranscodeTileHeaderResult* result)
{
    const JxrTranscodeTileQuantizerState* quantizers;

    if (state == NULL || result == NULL || state->quantizers == NULL ||
        state->dcOutput == NULL || state->channelCount == 0 ||
        state->channelCount > MAX_CHANNELS ||
        (state->hasAlpha && state->alphaChannelIndex >= MAX_CHANNELS)) return FALSE;
    quantizers = state->quantizers;
    result->lowpassQuantizerBits = quantizers->useDcForLowpass ? 0 :
        dquantBits(quantizers->lowpassQuantizerCount);
    result->highpassQuantizerBits = quantizers->useLowpassForHighpass ? 0 :
        dquantBits(quantizers->highpassQuantizerCount);
    result->lowpassAlphaQuantizerBits = quantizers->useDcForLowpassAlpha ? 0 :
        dquantBits(quantizers->lowpassQuantizerCountAlpha);
    result->highpassAlphaQuantizerBits = quantizers->useLowpassForHighpassAlpha ? 0 :
        dquantBits(quantizers->highpassQuantizerCountAlpha);

    if (!JxrTranscodeTileHeaderWriterWritePacket(state->dcOutput,
        state->isSpatial ? 0 : 1, state->tileId)) return FALSE;
    if (state->trimFlexbits && state->isSpatial &&
        !JxrTranscodeBitSinkWriteBits(state->dcOutput, state->trimFlexbitsValue, 4)) return FALSE;
    if (!JxrTranscodeTileHeaderWriterWriteDc(state, state->dcOutput)) return FALSE;
    if (state->subband == SB_DC_ONLY) return TRUE;

    if (state->isSpatial) {
        if (!JxrTranscodeTileHeaderWriterWriteLowpass(state, state->dcOutput)) return FALSE;
        if (state->subband != SB_NO_HIGHPASS &&
            !JxrTranscodeTileHeaderWriterWriteHighpass(state, state->dcOutput)) return FALSE;
        return TRUE;
    }

    if (state->lowpassOutput == NULL ||
        !JxrTranscodeTileHeaderWriterWritePacket(state->lowpassOutput, 2, state->tileId) ||
        !JxrTranscodeTileHeaderWriterWriteLowpass(state, state->lowpassOutput)) return FALSE;
    if (state->subband == SB_NO_HIGHPASS) return TRUE;
    if (state->highpassOutput == NULL ||
        !JxrTranscodeTileHeaderWriterWritePacket(state->highpassOutput, 3, state->tileId) ||
        !JxrTranscodeTileHeaderWriterWriteHighpass(state, state->highpassOutput)) return FALSE;
    if (state->subband != SB_NO_FLEXBITS &&
        (state->flexbitsOutput == NULL ||
        !JxrTranscodeTileHeaderWriterWritePacket(state->flexbitsOutput, 4, state->tileId) ||
        (state->trimFlexbits && !JxrTranscodeBitSinkWriteBits(state->flexbitsOutput,
            state->trimFlexbitsValue, 4)))) return FALSE;
    return TRUE;
}
