#include "JxrEncoderChromaResamplingSetup.h"

Void JxrEncoderChromaResamplingPlanInitialize(JxrEncoderChromaResamplingPlan* plan,
    COLORFORMAT sourceFormat, COLORFORMAT targetFormat, Bool inputIsYuvData,
    size_t macroblockWidth, Bool isThirtyTwoBitBuild)
{
    plan->changesUvResolution = (((sourceFormat == CF_RGB || sourceFormat == YUV_444 ||
        sourceFormat == CMYK || sourceFormat == CF_RGBE) &&
        (targetFormat == YUV_422 || targetFormat == YUV_420)) ||
        (sourceFormat == YUV_422 && targetFormat == YUV_420)) && !inputIsYuvData;
    plan->allocationIsSafe = TRUE;
    plan->residualRowStride = 0;
    plan->residualSampleCount = 0;

    if (!plan->changesUvResolution)
        return;

    plan->residualRowStride = (sourceFormat == YUV_422 ? 128 : 256) +
        (targetFormat == YUV_420 ? 32 : 0);

    if (isThirtyTwoBitBuild &&
        (((macroblockWidth >> 16) * plan->residualRowStride) & 0xffff0000) != 0) {
        plan->allocationIsSafe = FALSE;
        return;
    }

    plan->residualSampleCount = plan->residualRowStride * macroblockWidth + 256;
    if (isThirtyTwoBitBuild && plan->residualSampleCount >= 0x3fffffff)
        plan->allocationIsSafe = FALSE;
}

Int JxrEncoderChromaResamplingSetupInitialize(CWMImageStrCodec* codec)
{
    JxrEncoderChromaResamplingPlan plan;

    JxrEncoderChromaResamplingPlanInitialize(&plan, codec->WMII.cfColorFormat,
        codec->m_param.cfColorFormat, codec->WMISCP.bYUVData, codec->cmbWidth,
        sizeof(size_t) == 4);
    codec->m_bUVResolutionChange = plan.changesUvResolution;

    if (!plan.changesUvResolution)
        return ICERR_OK;
    if (!plan.allocationIsSafe)
        return ICERR_ERROR;

    codec->pResU = (PixelI*)malloc(plan.residualSampleCount * sizeof(PixelI));
    codec->pResV = (PixelI*)malloc(plan.residualSampleCount * sizeof(PixelI));
    if (codec->pResU == NULL || codec->pResV == NULL)
        return ICERR_ERROR;

    return ICERR_OK;
}
