#include "JxrTranscodeAlphaPlaneInitializer.h"
#include "JxrDecoderResourceInitializer.h"
#include "JxrTranscodeSecondaryPlaneSetup.h"
#include "JxrTranscodeSecondaryPlaneLink.h"

EXTERN_C Int ReadImagePlaneHeader(CWMImageInfo*, CWMIStrCodecParam*,
    CCoreParameters*, SimpleBitIO*);
EXTERN_C Int WriteImagePlaneHeader(CWMImageStrCodec*);
EXTERN_C Int StrEncInit(CWMImageStrCodec*);

Int JxrTranscodeAlphaPlaneInitializerInitializeDecoder(
    CWMImageStrCodec* decoderCodec, CWMTranscodingParam* parameters,
    PixelI* alphaMacroblockBuffer,
    JxrTranscodeAlphaPlaneInitializationResult* result)
{
    SimpleBitIO bitInput = {0};
    CWMImageStrCodec* secondaryCodec;
    JxrTranscodeSecondaryPlaneLink secondaryLink;

    if (decoderCodec == NULL || parameters == NULL || result == NULL)
        return ICERR_ERROR;
    memset(result, 0, sizeof(*result));
    if (!decoderCodec->m_param.bAlphaChannel) {
        parameters->uAlphaMode = 0;
        return ICERR_OK;
    }
    if (alphaMacroblockBuffer == NULL || decoderCodec->WMISCP.pWStream == NULL)
        return ICERR_ERROR;
    result->hasAlpha = TRUE;
    result->channelIndex = decoderCodec->m_param.cNumChannels;
    if (JxrTranscodeSecondaryPlaneSetupCreate(decoderCodec,
        &secondaryCodec) != ICERR_OK)
        return ICERR_ERROR;
    JxrTranscodeSecondaryPlaneLinkInitialize(&secondaryLink);
    if (!JxrTranscodeSecondaryPlaneLinkAttach(&secondaryLink, decoderCodec,
        secondaryCodec)) {
        free(secondaryCodec);
        return ICERR_ERROR;
    }
    secondaryCodec->p1MBbuffer[0] = alphaMacroblockBuffer;

    if (attach_SB(&bitInput, decoderCodec->WMISCP.pWStream) != ICERR_OK) {
        JxrTranscodeSecondaryPlaneLinkDetachAndRelease(&secondaryLink);
        return ICERR_ERROR;
    }
    ReadImagePlaneHeader(&secondaryCodec->WMII, &secondaryCodec->WMISCP,
        &secondaryCodec->m_param, &bitInput);
    detach_SB(&bitInput);
    if (JxrDecoderResourceInitializerInitialize(secondaryCodec) != ICERR_OK) {
        JxrTranscodeSecondaryPlaneLinkDetachAndRelease(&secondaryLink);
        return ICERR_ERROR;
    }
    return ICERR_OK;
}

Int JxrTranscodeAlphaPlaneInitializerFinalizeDecoder(
    CWMImageStrCodec* decoderCodec)
{
    if (decoderCodec == NULL) return ICERR_ERROR;
    if (!decoderCodec->m_param.bAlphaChannel) return ICERR_OK;
    if (decoderCodec->m_pNextSC == NULL) return ICERR_ERROR;
    return JxrDecoderResourceInitializerInitialize(decoderCodec->m_pNextSC);
}

Int JxrTranscodeAlphaPlaneInitializerInitializeEncoder(
    CWMImageStrCodec* decoderCodec, CWMImageStrCodec* encoderCodec,
    const CWMTranscodingParam* parameters)
{
    CWMImageStrCodec* secondaryCodec;
    JxrTranscodeSecondaryPlaneLink secondaryLink;

    if (decoderCodec == NULL || encoderCodec == NULL || parameters == NULL)
        return ICERR_ERROR;
    if (parameters->uAlphaMode == 0) return ICERR_OK;
    if (decoderCodec->m_pNextSC == NULL) return ICERR_ERROR;
    if (JxrTranscodeSecondaryPlaneSetupCreate(encoderCodec,
        &secondaryCodec) != ICERR_OK)
        return ICERR_ERROR;
    JxrTranscodeSecondaryPlaneLinkInitialize(&secondaryLink);
    if (!JxrTranscodeSecondaryPlaneLinkAttach(&secondaryLink, encoderCodec,
        secondaryCodec)) {
        free(secondaryCodec);
        return ICERR_ERROR;
    }
    secondaryCodec->pPlane[0] = decoderCodec->m_pNextSC->p1MBbuffer[0];
    secondaryCodec->m_param = decoderCodec->m_pNextSC->m_param;
    encoderCodec->m_param.bAlphaChannel = TRUE;

    if (parameters->bIgnoreOverlap)
        secondaryCodec->pTile = decoderCodec->m_pNextSC->pTile;
    else if (StrEncInit(secondaryCodec) != ICERR_OK) {
        JxrTranscodeSecondaryPlaneLinkDetachAndRelease(&secondaryLink);
        return ICERR_ERROR;
    }
    WriteImagePlaneHeader(secondaryCodec);
    return ICERR_OK;
}
