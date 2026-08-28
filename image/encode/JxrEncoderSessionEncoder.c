#include "JxrEncoderSessionEncoder.h"
#include "JxrEncoderProcessingPipeline.h"
#include "JxrEncoderMacroblockProcessingPipeline.h"
#include "JXRTrace.h"
#include "perfTimer.h"

static Void JxrEncoderSessionEncoderInitializeProcessingPlan(
    JxrEncoderProcessingPipelinePlan* processingPlan)
{
#if defined(WMP_OPT_SSE2) || defined(WMP_OPT_CC_ENC) || defined(WMP_OPT_TRFM_ENC)
    JxrEncoderProcessingPipelinePlanInitialize(processingPlan, TRUE);
#else
    JxrEncoderProcessingPipelinePlanInitialize(processingPlan, FALSE);
#endif
}

static Int JxrEncoderSessionEncoderLoadInput(CWMImageStrCodec* codec,
    const JxrEncoderProcessingPipelinePlan* processingPlan)
{
    if (processingPlan->usesLegacyLoadCallback)
        return codec->Load(codec);

    return JxrEncoderProcessingPipelineLoadInput(codec);
}

Int JxrEncoderSessionEncoderEncodeRow(CWMImageStrCodec* codec,
    const CWMImageBufferInfo* bufferInfo)
{
    CWMImageStrCodec* nextCodec;
    JxrEncoderProcessingPipelinePlan processingPlan;

    JxrEncoderSessionEncoderInitializeProcessingPlan(&processingPlan);

    if (sizeof(*codec) != codec->cbStruct)
        return ICERR_ERROR;

    PERFTIMER_START(codec->m_fMeasurePerf, codec->m_ptEncDecPerf);

    codec->WMIBI = *bufferInfo;
    codec->cColumn = 0;
    initMRPtr(codec);

    nextCodec = codec->m_pNextSC;
    if (nextCodec)
        nextCodec->WMIBI = *bufferInfo;

    if (JxrEncoderSessionEncoderLoadInput(codec, &processingPlan) != ICERR_OK)
        return ICERR_ERROR;

    JXRTraceDumpStage("encoder", "centered_samples", codec, 0,
        (Int)codec->cRow, JXRTraceSamples);

    if (JxrEncoderMacroblockProcessingPipelineProcessLoadedRow(codec) != ICERR_OK)
        return ICERR_ERROR;

    PERFTIMER_STOP(codec->m_fMeasurePerf, codec->m_ptEncDecPerf);
    return ICERR_OK;
}
