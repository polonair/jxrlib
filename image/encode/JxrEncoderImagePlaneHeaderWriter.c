#include "JxrEncoderImagePlaneHeaderWriter.h"

#include "JxrEncoderTileHeaderWriter.h"

Void JxrEncoderImagePlaneHeaderQuantizerPlanInitialize(
    JxrEncoderImagePlaneHeaderQuantizerPlan* plan,
    U32 quantizerMode,
    SUBBAND subband)
{
    plan->writesDcFrameQuantizer = (quantizerMode & 1) == 0;
    plan->writesLowpassSyntax = subband != SB_DC_ONLY;
    plan->lowpassUsesDcQuantizer = (quantizerMode & 0x200) == 0;
    plan->writesLpFrameQuantizer = plan->writesLowpassSyntax &&
        !plan->lowpassUsesDcQuantizer && (quantizerMode & 2) == 0;
    plan->writesHighpassSyntax = plan->writesLowpassSyntax &&
        subband != SB_NO_HIGHPASS;
    plan->highpassUsesLpQuantizer = (quantizerMode & 0x400) == 0;
    plan->writesHpFrameQuantizer = plan->writesHighpassSyntax &&
        !plan->highpassUsesLpQuantizer && (quantizerMode & 4) == 0;
}

static Void JxrEncoderImagePlaneHeaderWriterWriteColorParameters(
    const CWMImageStrCodec* codec,
    BitIOInfo* bitWriter)
{
    switch (codec->m_param.cfColorFormat) {
        case YUV_420:
        case YUV_422:
        case YUV_444:
            putBit16(bitWriter, 0, 4);
            putBit16(bitWriter, 0, 4);
            break;
        case NCOMPONENT:
            putBit16(bitWriter, (Int)codec->m_param.cNumChannels - 1, 4);
            putBit16(bitWriter, 0, 4);
            break;
        default:
            break;
    }
}

static Void JxrEncoderImagePlaneHeaderWriterWriteBitDepthParameters(
    CWMImageStrCodec* codec,
    BitIOInfo* bitWriter)
{
    CWMIStrCodecParam* parameters = &codec->WMISCP;

    switch (codec->WMII.bdBitDepth) {
        case BD_16:
        case BD_16S:
            putBit16(bitWriter, parameters->nLenMantissaOrShift, 8);
            break;
        case BD_32:
        case BD_32S:
            if (parameters->nLenMantissaOrShift == 0)
                parameters->nLenMantissaOrShift = 10;
            putBit16(bitWriter, parameters->nLenMantissaOrShift, 8);
            break;
        case BD_32F:
            if (parameters->nLenMantissaOrShift == 0)
                parameters->nLenMantissaOrShift = 13;
            putBit16(bitWriter, parameters->nLenMantissaOrShift, 8);
            putBit16(bitWriter, parameters->nExpBias, 8);
            break;
        default:
            break;
    }
}

static Void JxrEncoderImagePlaneHeaderWriterWriteQuantizers(
    CWMImageStrCodec* codec,
    BitIOInfo* bitWriter)
{
    JxrEncoderImagePlaneHeaderQuantizerPlan plan;
    U32 quantizerMode = codec->m_param.uQPMode;

    JxrEncoderImagePlaneHeaderQuantizerPlanInitialize(&plan, quantizerMode,
        codec->WMISCP.sbSubband);
    putBit16(bitWriter, plan.writesDcFrameQuantizer ? 1 : 0, 1);
    if (plan.writesDcFrameQuantizer)
        JxrEncoderTileHeaderWriterWriteQuantizer(codec->pTile[0].pQuantizerDC,
            bitWriter, (quantizerMode >> 3) & 3, codec->m_param.cNumChannels, 0);

    if (!plan.writesLowpassSyntax)
        return;
    putBit16(bitWriter, plan.lowpassUsesDcQuantizer ? 1 : 0, 1);
    if (!plan.lowpassUsesDcQuantizer) {
        putBit16(bitWriter, plan.writesLpFrameQuantizer ? 1 : 0, 1);
        if (plan.writesLpFrameQuantizer)
            JxrEncoderTileHeaderWriterWriteQuantizer(codec->pTile[0].pQuantizerLP,
                bitWriter, (quantizerMode >> 5) & 3, codec->m_param.cNumChannels, 0);
    }

    if (!plan.writesHighpassSyntax)
        return;
    putBit16(bitWriter, plan.highpassUsesLpQuantizer ? 1 : 0, 1);
    if (!plan.highpassUsesLpQuantizer) {
        putBit16(bitWriter, plan.writesHpFrameQuantizer ? 1 : 0, 1);
        if (plan.writesHpFrameQuantizer)
            JxrEncoderTileHeaderWriterWriteQuantizer(codec->pTile[0].pQuantizerHP,
                bitWriter, (quantizerMode >> 7) & 3, codec->m_param.cNumChannels, 0);
    }
}

Int JxrEncoderImagePlaneHeaderWriterWrite(CWMImageStrCodec* codec)
{
    BitIOInfo* bitWriter = codec->pIOHeader;

    putBit16(bitWriter, (Int)codec->m_param.cfColorFormat, 3);
    putBit16(bitWriter, (Int)codec->m_param.bScaledArith, 1);
    putBit16(bitWriter, (U32)codec->WMISCP.sbSubband, 4);
    JxrEncoderImagePlaneHeaderWriterWriteColorParameters(codec, bitWriter);
    JxrEncoderImagePlaneHeaderWriterWriteBitDepthParameters(codec, bitWriter);
    JxrEncoderImagePlaneHeaderWriterWriteQuantizers(codec, bitWriter);
    fillToByte(bitWriter);
    return ICERR_OK;
}
