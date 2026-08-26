#include "JxrSecondaryPlaneInitializer.h"
#include "JxrHeaderDecodePipeline.h"
#include "JxrSecondaryPlaneFactory.h"

Void JxrSecondaryPlaneInitializerInit(JxrSecondaryPlaneInitializer* initializer,
    CWMImageStrCodec* primaryCodec, const CCoreParameters* templateParameters,
    const CWMImageStrCodec* templateCodec, size_t channelBytes, size_t macroblockCount)
{
    initializer->primaryCodec = primaryCodec;
    initializer->templateParameters = templateParameters;
    initializer->templateCodec = templateCodec;
    initializer->channelBytes = channelBytes;
    initializer->macroblockCount = macroblockCount;
}

Int JxrSecondaryPlaneInitializerRun(JxrSecondaryPlaneInitializer* initializer,
    CWMImageStrCodec** secondaryCodec)
{
    CWMImageStrCodec* secondary;
    Bool isAttached = FALSE;
    Int result;
    SimpleBitIO bitInput = {0};
    if (secondaryCodec != NULL) *secondaryCodec = NULL;
    if (initializer == NULL || secondaryCodec == NULL || initializer->primaryCodec == NULL ||
        initializer->templateParameters == NULL || initializer->templateCodec == NULL ||
        initializer->primaryCodec->WMISCP.pWStream == NULL) return ICERR_ERROR;
    result = JxrSecondaryPlaneFactoryCreate(initializer->templateParameters,
        initializer->templateCodec, initializer->channelBytes, initializer->macroblockCount,
        &secondary);
    if (result != ICERR_OK) return result;
    result = attach_SB(&bitInput, initializer->primaryCodec->WMISCP.pWStream);
    if (result != WMP_errSuccess) {
        JxrSecondaryPlaneFactoryRelease(secondary);
        return result;
    }
    isAttached = TRUE;
    result = JxrHeaderDecodePipelineReadImagePlane(&secondary->WMII,
        &secondary->WMISCP, &secondary->m_param, &bitInput) ? ICERR_OK : ICERR_ERROR;
    if (result == ICERR_OK) {
        result = detach_SB(&bitInput);
        isAttached = FALSE;
    }
    if (result != ICERR_OK) {
        if (isAttached) {
            flushToByte_SB(&bitInput);
            detach_SB(&bitInput);
        }
        JxrSecondaryPlaneFactoryRelease(secondary);
        return result;
    }
    secondary->m_Dparam = initializer->primaryCodec->m_Dparam;
    secondary->cbChannel = initializer->channelBytes;
    secondary->m_param.cfColorFormat = Y_ONLY;
    secondary->m_param.cNumChannels = 1;
    secondary->m_param.bAlphaChannel = TRUE;
    secondary->pIOHeader = initializer->primaryCodec->pIOHeader;
    secondary->m_pNextSC = initializer->primaryCodec;
    secondary->m_bSecondary = TRUE;
    *secondaryCodec = secondary;
    return ICERR_OK;
}
