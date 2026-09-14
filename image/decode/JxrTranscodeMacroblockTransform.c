#include "JxrTranscodeMacroblockTransform.h"

#include "JxrTranscodeCoefficientTransform.h"

static Bool JxrTranscodeMacroblockTransformIsValid(
    const JxrTranscodeMacroblockTransformState* state)
{
    return state != NULL && state->sourceMacroblocks != NULL &&
        state->sourceCoefficients != NULL && state->destinationCodec != NULL &&
        state->destinationCoefficients != NULL && state->orientation != NULL;
}

static Bool JxrTranscodeMacroblockTransformChannel(
    const JxrTranscodeMacroblockTransformState* state, size_t channel,
    size_t dcCount, size_t coefficientOffset, size_t coefficientCount,
    Bool (*transformDc)(JxrTranscodeCoefficientBuffer*,
        JxrTranscodeCoefficientBuffer*, const JxrTranscodeOrientationState*),
    Bool (*transformAc)(JxrTranscodeCoefficientBuffer*,
        JxrTranscodeCoefficientBuffer*, const JxrTranscodeOrientationState*))
{
    JxrTranscodeCoefficientBuffer sourceDc;
    JxrTranscodeCoefficientBuffer destinationDc;
    JxrTranscodeCoefficientBuffer sourceAc;
    JxrTranscodeCoefficientBuffer destinationAc;
    const CWMIMBInfo* source = state->sourceMacroblocks + state->macroblockOffset;
    PixelI* sourceCoefficients = state->sourceCoefficients +
        state->macroblockOffset * state->coefficientUnit + coefficientOffset;
    PixelI* destinationCoefficients = state->destinationCoefficients + coefficientOffset;

    JxrTranscodeCoefficientBufferInit(&sourceDc, (PixelI*)source->iBlockDC[channel], 0, dcCount);
    JxrTranscodeCoefficientBufferInit(&destinationDc,
        state->destinationCodec->MBInfo.iBlockDC[channel], 0, dcCount);
    JxrTranscodeCoefficientBufferInit(&sourceAc, sourceCoefficients, 0, coefficientCount);
    JxrTranscodeCoefficientBufferInit(&destinationAc, destinationCoefficients, 0, coefficientCount);
    return transformDc(&sourceDc, &destinationDc, state->orientation) &&
        transformAc(&sourceAc, &destinationAc, state->orientation);
}

Bool JxrTranscodeMacroblockTransformPrimary(
    const JxrTranscodeMacroblockTransformState* state)
{
    size_t channel;
    size_t fullResolutionChannels;

    if (!JxrTranscodeMacroblockTransformIsValid(state)) return FALSE;
    fullResolutionChannels = state->destinationCodec->m_param.cfColorFormat == YUV_420 ||
        state->destinationCodec->m_param.cfColorFormat == YUV_422 ? 1 :
        state->destinationCodec->m_param.cNumChannels;
    for (channel = 0; channel < fullResolutionChannels; ++channel)
        if (!JxrTranscodeMacroblockTransformChannel(state, channel, 16, channel * 256, 256,
            JxrTranscodeCoefficientTransformDc444,
            JxrTranscodeCoefficientTransformAc444)) return FALSE;

    if (state->destinationCodec->WMISCP.cfColorFormat == YUV_420) {
        for (channel = 0; channel < 2; ++channel)
            if (!JxrTranscodeMacroblockTransformChannel(state, channel + 1, 4,
                256 + channel * 64, 64, JxrTranscodeCoefficientTransformDc420,
                JxrTranscodeCoefficientTransformAc420)) return FALSE;
    }
    else if (state->destinationCodec->WMISCP.cfColorFormat == YUV_422) {
        for (channel = 0; channel < 2; ++channel)
            if (!JxrTranscodeMacroblockTransformChannel(state, channel + 1, 8,
                256 + channel * 128, 128, JxrTranscodeCoefficientTransformDc422,
                JxrTranscodeCoefficientTransformAc422)) return FALSE;
        state->destinationCodec->MBInfo.iQIndexLP =
            state->sourceMacroblocks[state->macroblockOffset].iQIndexLP;
        state->destinationCodec->MBInfo.iQIndexHP =
            state->sourceMacroblocks[state->macroblockOffset].iQIndexHP;
    }
    return TRUE;
}

Bool JxrTranscodeMacroblockTransformAlpha(
    const JxrTranscodeMacroblockTransformState* state)
{
    if (!JxrTranscodeMacroblockTransformIsValid(state)) return FALSE;
    if (!JxrTranscodeMacroblockTransformChannel(state, 0, 16, 0, 256,
        JxrTranscodeCoefficientTransformDc444,
        JxrTranscodeCoefficientTransformAc444)) return FALSE;
    state->destinationCodec->MBInfo.iQIndexLP =
        state->sourceMacroblocks[state->macroblockOffset].iQIndexLP;
    state->destinationCodec->MBInfo.iQIndexHP =
        state->sourceMacroblocks[state->macroblockOffset].iQIndexHP;
    return TRUE;
}
