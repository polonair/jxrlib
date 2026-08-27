#include "JxrDecoderSessionFactory.h"
#include "JxrDecoderMemoryLayoutPlan.h"
#include "JxrDecoderPrimaryPlaneFactory.h"
#include "JxrDecoderInitializationPipeline.h"
#include "JxrSecondaryPlaneInitializer.h"
#include "decode.h"
#include "perfTimer.h"

Int JxrDecoderSessionFactoryCreate(const JxrDecoderSessionPreparation* preparation,
    Bool measuresPerformance, CWMImageInfo* imageInfo,
    CWMIStrCodecParam* codecParameters, CWMImageStrCodec** primaryCodec)
{
    JxrDecoderMemoryLayoutPlan memoryLayout;
    CWMImageStrCodec* codec;
    CWMImageStrCodec* secondaryCodec = NULL;
    size_t macroblockCount;
    Bool isThirtyTwoBitBuild = sizeof(void*) < 8;
    Int result;

    if (primaryCodec != NULL) *primaryCodec = NULL;
    if (preparation == NULL || imageInfo == NULL || codecParameters == NULL ||
        primaryCodec == NULL) return ICERR_ERROR;

    JxrDecoderMemoryLayoutPlanInitialize(&memoryLayout,
        preparation->templateCodec.WMISCP.bdBitDepth,
        preparation->templateCodec.m_param.cfColorFormat,
        preparation->templateCodec.m_param.cNumChannels,
        preparation->templateCodec.WMII.cWidth, sizeof(*codec),
        sizeof(CWMDecoderParameters), sizeof(BitIOInfo), isThirtyTwoBitBuild);
    if (!memoryLayout.allocationIsSafe) return ICERR_ERROR;
    macroblockCount = memoryLayout.macroblockCount;

    result = JxrDecoderPrimaryPlaneFactoryCreate(&memoryLayout,
        &preparation->templateCodec.m_param, &preparation->templateCodec,
        preparation->usesHardTileBoundaries, measuresPerformance, &codec);
    if (result != ICERR_OK) return result;

    if (codec->m_param.bAlphaChannel) {
        JxrSecondaryPlaneInitializer secondaryInitializer;

        JxrSecondaryPlaneInitializerInit(&secondaryInitializer, codec,
            &preparation->templateCodec.m_param, &preparation->templateCodec,
            memoryLayout.channelBytes, macroblockCount);
        result = JxrSecondaryPlaneInitializerRun(&secondaryInitializer, &secondaryCodec);
        if (result != ICERR_OK) return result;
    }
    else
        codec->WMISCP.uAlphaMode = 0;

    {
        JxrDecoderInitializationPipeline initialization;

        JxrDecoderInitializationPipelineInit(&initialization, codec, secondaryCodec);
        if (JxrDecoderInitializationPipelineRun(&initialization) != ICERR_OK)
            return ICERR_ERROR;
    }

    *imageInfo = codec->WMII;
    *codecParameters = codec->WMISCP;
    if (codec->WMII.cPostProcStrength) {
        initPostProc(codec->pPostProcInfo, codec->cmbWidth, codec->m_param.cNumChannels);
        if (codec->m_param.bAlphaChannel)
            initPostProc(secondaryCodec->pPostProcInfo, secondaryCodec->cmbWidth,
                secondaryCodec->m_param.cNumChannels);
    }
    PERFTIMER_STOP(codec->m_fMeasurePerf, codec->m_ptEncDecPerf);
    *primaryCodec = codec;
    return ICERR_OK;
}
