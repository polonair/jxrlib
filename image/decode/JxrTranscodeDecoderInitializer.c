#include "JxrTranscodeDecoderInitializer.h"

EXTERN_C Int ReadWMIHeader(CWMImageInfo*, CWMIStrCodecParam*, CCoreParameters*);

Int JxrTranscodeDecoderInitializerInitialize(CWMImageStrCodec* decoderCodec,
    CWMTranscodingParam* parameters, CWMDecoderParameters* decoderParameters,
    JxrTranscodeDecoderInitializationResult* result)
{
    size_t coefficientUnit;

    if (decoderCodec == NULL || parameters == NULL || decoderParameters == NULL ||
        result == NULL) return ICERR_ERROR;
    if (ReadWMIHeader(&decoderCodec->WMII, &decoderCodec->WMISCP,
        &decoderCodec->m_param) != ICERR_OK) return ICERR_ERROR;
    result->orientationValue = parameters->oOrientation;
    JxrTranscodeOrientationStateInit(&result->orientation, result->orientationValue);
    if (decoderCodec->WMISCP.cfColorFormat == YUV_422 && result->orientation.transpose) {
        parameters->oOrientation = result->orientationValue = O_NONE;
        JxrTranscodeOrientationStateInit(&result->orientation, result->orientationValue);
    }
    decoderCodec->cmbWidth = (decoderCodec->WMII.cWidth +
        decoderCodec->m_param.cExtraPixelsLeft + decoderCodec->m_param.cExtraPixelsRight + 15) / 16;
    decoderCodec->cmbHeight = (decoderCodec->WMII.cHeight +
        decoderCodec->m_param.cExtraPixelsTop + decoderCodec->m_param.cExtraPixelsBottom + 15) / 16;
    decoderCodec->m_param.cNumChannels = decoderCodec->WMISCP.cChannel;
    decoderCodec->m_Dparam = decoderParameters;
    decoderParameters->bSkipFlexbits = decoderCodec->WMISCP.sbSubband == SB_NO_FLEXBITS;
    decoderCodec->m_param.bTranscode = TRUE;
    coefficientUnit = decoderCodec->m_param.cfColorFormat == YUV_420 ? 384 :
        (decoderCodec->m_param.cfColorFormat == YUV_422 ? 512 :
            256 * decoderCodec->m_param.cNumChannels);
    if (coefficientUnit > 256 * MAX_CHANNELS) return ICERR_ERROR;
    result->coefficientUnit = coefficientUnit;
    return ICERR_OK;
}
