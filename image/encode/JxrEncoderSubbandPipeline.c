#include "JxrEncoderSubbandPipeline.h"

#include "JXRTrace.h"
#include "encode.h"

Void JxrEncoderSubbandPlanInitialize(
    JxrEncoderSubbandPlan* plan,
    SUBBAND subband)
{
    plan->encodesLowpass = subband != SB_DC_ONLY;
    plan->encodesHighpass = plan->encodesLowpass && subband != SB_NO_HIGHPASS;
}

static Int JxrEncoderSubbandPipelineEncodeDc(
    CWMImageStrCodec* codec,
    CCodingContext* codingContext,
    Int macroblockX,
    Int macroblockY)
{
    size_t bitStart = JXRTraceBitPosition(codingContext->m_pIODC, TRUE);

    if (EncodeMacroblockDC(codec, codingContext, macroblockX, macroblockY) != ICERR_OK)
        return ICERR_ERROR;

    JXRTraceDumpBitRange("encoder", "dc", macroblockX, macroblockY, bitStart,
        JXRTraceBitPosition(codingContext->m_pIODC, TRUE));
    return ICERR_OK;
}

static Int JxrEncoderSubbandPipelineEncodeLowpass(
    CWMImageStrCodec* codec,
    CCodingContext* codingContext,
    Int macroblockX,
    Int macroblockY)
{
    size_t bitStart = JXRTraceBitPosition(codingContext->m_pIOLP, TRUE);

    if (EncodeMacroblockLowpass(codec, codingContext, macroblockX, macroblockY) != ICERR_OK)
        return ICERR_ERROR;

    JXRTraceDumpBitRange("encoder", "lp", macroblockX, macroblockY, bitStart,
        JXRTraceBitPosition(codingContext->m_pIOLP, TRUE));
    return ICERR_OK;
}

static Int JxrEncoderSubbandPipelineEncodeHighpass(
    CWMImageStrCodec* codec,
    CCodingContext* codingContext,
    Int macroblockX,
    Int macroblockY)
{
    size_t bitStart = JXRTraceBitPosition(codingContext->m_pIOAC, TRUE);

    if (EncodeMacroblockHighpass(codec, codingContext, macroblockX, macroblockY) != ICERR_OK)
        return ICERR_ERROR;

    JXRTraceDumpBitRange("encoder", "hp", macroblockX, macroblockY, bitStart,
        JXRTraceBitPosition(codingContext->m_pIOAC, TRUE));
    return ICERR_OK;
}

Int JxrEncoderSubbandPipelineProcess(
    CWMImageStrCodec* codec,
    CCodingContext* codingContext,
    Int macroblockX,
    Int macroblockY)
{
    JxrEncoderSubbandPlan plan;

    JxrEncoderSubbandPlanInitialize(&plan, codec->WMISCP.sbSubband);
    if (JxrEncoderSubbandPipelineEncodeDc(codec, codingContext, macroblockX, macroblockY) != ICERR_OK)
        return ICERR_ERROR;
    if (plan.encodesLowpass &&
        JxrEncoderSubbandPipelineEncodeLowpass(codec, codingContext, macroblockX, macroblockY) != ICERR_OK)
        return ICERR_ERROR;
    if (plan.encodesHighpass &&
        JxrEncoderSubbandPipelineEncodeHighpass(codec, codingContext, macroblockX, macroblockY) != ICERR_OK)
        return ICERR_ERROR;

    return ICERR_OK;
}
