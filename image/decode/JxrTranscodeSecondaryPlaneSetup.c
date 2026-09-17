#include "JxrTranscodeSecondaryPlaneSetup.h"

static Void JxrTranscodeSecondaryPlaneSetupCopySharedState(
    CWMImageStrCodec* secondaryCodec, const CWMImageStrCodec* primaryCodec)
{
    secondaryCodec->WMII = primaryCodec->WMII;
    secondaryCodec->WMISCP = primaryCodec->WMISCP;
    secondaryCodec->WMIBI = primaryCodec->WMIBI;
    secondaryCodec->m_param = primaryCodec->m_param;
    secondaryCodec->m_Dparam = primaryCodec->m_Dparam;
    secondaryCodec->cSB = primaryCodec->cSB;
    secondaryCodec->m_bUVResolutionChange = primaryCodec->m_bUVResolutionChange;
    secondaryCodec->bTileExtraction = primaryCodec->bTileExtraction;
    secondaryCodec->bUseHardTileBoundaries = primaryCodec->bUseHardTileBoundaries;
    secondaryCodec->cmbWidth = primaryCodec->cmbWidth;
    secondaryCodec->cmbHeight = primaryCodec->cmbHeight;
    secondaryCodec->cbChannel = primaryCodec->cbChannel;
    secondaryCodec->Load = primaryCodec->Load;
    secondaryCodec->m_bDecoderUseAlternateTransform =
        primaryCodec->m_bDecoderUseAlternateTransform;
    secondaryCodec->m_bDecoderUseCenterTransform =
        primaryCodec->m_bDecoderUseCenterTransform;
    secondaryCodec->TransformCenter = primaryCodec->TransformCenter;
}

Int JxrTranscodeSecondaryPlaneSetupCreate(CWMImageStrCodec* primaryCodec,
    CWMImageStrCodec** secondaryCodec)
{
    CWMImageStrCodec* secondary;

    if (secondaryCodec != NULL) *secondaryCodec = NULL;
    if (primaryCodec == NULL || secondaryCodec == NULL) return ICERR_ERROR;
    secondary = (CWMImageStrCodec*)malloc(sizeof(CWMImageStrCodec));
    if (secondary == NULL) return ICERR_ERROR;
    memset(secondary, 0, sizeof(CWMImageStrCodec));
    JxrTranscodeSecondaryPlaneSetupCopySharedState(secondary, primaryCodec);
    secondary->WMISCP.cfColorFormat = Y_ONLY;
    secondary->WMII.cfColorFormat = Y_ONLY;
    secondary->m_param.cfColorFormat = Y_ONLY;
    secondary->WMISCP.cChannel = 1;
    secondary->m_param.cNumChannels = 1;
    secondary->m_bSecondary = TRUE;
    secondary->m_pNextSC = primaryCodec;
    *secondaryCodec = secondary;
    return ICERR_OK;
}
