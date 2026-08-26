#include "JxrSecondaryPlaneInitializer.h"
#include "JxrDecoderCodecStateInitializer.h"
#include "JxrHeaderDecodePipeline.h"
#include "JxrSecondaryPlaneMemoryLayoutPlan.h"
#include "JxrSecondaryPlaneBufferRegionLayout.h"
#include <stdlib.h>
#include <string.h>

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
    U8* storage;
    JxrSecondaryPlaneMemoryLayoutPlan memoryLayout;
    JxrSecondaryPlaneBufferRegionLayout bufferLayout;
    Bool isAttached = FALSE;
    Int result;
    SimpleBitIO bitInput = {0};
    if (secondaryCodec != NULL) *secondaryCodec = NULL;
    if (initializer == NULL || secondaryCodec == NULL || initializer->primaryCodec == NULL ||
        initializer->templateParameters == NULL || initializer->templateCodec == NULL ||
        initializer->primaryCodec->WMISCP.pWStream == NULL) return ICERR_ERROR;
    JxrSecondaryPlaneMemoryLayoutPlanInitialize(&memoryLayout,
        initializer->channelBytes, initializer->macroblockCount, sizeof(*secondary));
    storage = (U8*)malloc(memoryLayout.allocationBytes);
    if (storage == NULL) return WMP_errOutOfMemory;
    memset(storage, 0, memoryLayout.allocationBytes);
    JxrSecondaryPlaneBufferRegionLayoutInitialize(&bufferLayout, (UINTPTR_T)storage,
        sizeof(*secondary), &memoryLayout);
    secondary = (CWMImageStrCodec*)storage;
    JxrDecoderCodecStateInitializerInitialize(secondary, initializer->templateParameters,
        initializer->templateCodec);
    result = attach_SB(&bitInput, initializer->primaryCodec->WMISCP.pWStream);
    if (result != WMP_errSuccess) {
        free(storage);
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
        free(storage);
        return result;
    }
    secondary->m_Dparam = initializer->primaryCodec->m_Dparam;
    secondary->cbChannel = initializer->channelBytes;
    secondary->m_param.cfColorFormat = Y_ONLY;
    secondary->m_param.cNumChannels = 1;
    secondary->m_param.bAlphaChannel = TRUE;
    JxrSecondaryPlaneBufferRegionLayoutBind(secondary, storage, &bufferLayout,
        memoryLayout.macroblockStride);
    secondary->pIOHeader = initializer->primaryCodec->pIOHeader;
    secondary->m_pNextSC = initializer->primaryCodec;
    secondary->m_bSecondary = TRUE;
    *secondaryCodec = secondary;
    return ICERR_OK;
}
