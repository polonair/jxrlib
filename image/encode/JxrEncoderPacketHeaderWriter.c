#include "JxrEncoderPacketHeaderWriter.h"

#include "encode.h"

Void JxrEncoderPacketHeaderPlanInitialize(
    JxrEncoderPacketHeaderPlan* plan,
    BITSTREAMFORMAT bitstreamFormat,
    U8 subbandPacketCount,
    Bool trimFlexbits,
    Bool contextLeft,
    Bool contextTop,
    Bool isSecondary,
    Bool isTranscode)
{
    plan->writesHeaders = contextLeft && contextTop && !isSecondary && !isTranscode;
    plan->usesSpatialLayout = bitstreamFormat == SPATIAL;
    plan->writesLowpassHeader = plan->usesSpatialLayout || subbandPacketCount > 1;
    plan->writesHighpassHeader = plan->usesSpatialLayout || subbandPacketCount > 2;
    plan->writesFlexbitsPacket = !plan->usesSpatialLayout && subbandPacketCount > 3;
    plan->writesTrimFlexbits = trimFlexbits &&
        (plan->usesSpatialLayout || plan->writesFlexbitsPacket);
}

static U8 JxrEncoderPacketHeaderWriterGetPacketId(const CWMImageStrCodec* codec)
{
    return (U8)((codec->cTileRow * (codec->WMISCP.cNumOfSliceMinus1V + 1) +
        codec->cTileColumn) & 0x1F);
}

static Void JxrEncoderPacketHeaderWriterWriteSpatial(
    CWMImageStrCodec* codec,
    CCodingContext* codingContext,
    const JxrEncoderPacketHeaderPlan* plan,
    U8 packetId)
{
    writePacketHeader(codingContext->m_pIODC, 0, packetId);
    if (plan->writesTrimFlexbits)
        putBit16(codingContext->m_pIODC, codingContext->m_iTrimFlexBits, 4);
    writeTileHeaderDC(codec, codingContext->m_pIODC);
    writeTileHeaderLP(codec, codingContext->m_pIODC);
    writeTileHeaderHP(codec, codingContext->m_pIODC);
}

static Void JxrEncoderPacketHeaderWriterWriteFrequency(
    CWMImageStrCodec* codec,
    CCodingContext* codingContext,
    const JxrEncoderPacketHeaderPlan* plan,
    U8 packetId)
{
    writePacketHeader(codingContext->m_pIODC, 1, packetId);
    writeTileHeaderDC(codec, codingContext->m_pIODC);

    if (plan->writesLowpassHeader) {
        writePacketHeader(codingContext->m_pIOLP, 2, packetId);
        writeTileHeaderLP(codec, codingContext->m_pIOLP);
    }
    if (plan->writesHighpassHeader) {
        writePacketHeader(codingContext->m_pIOAC, 3, packetId);
        writeTileHeaderHP(codec, codingContext->m_pIOAC);
    }
    if (plan->writesFlexbitsPacket) {
        writePacketHeader(codingContext->m_pIOFL, 4, packetId);
        if (plan->writesTrimFlexbits)
            putBit16(codingContext->m_pIOFL, codingContext->m_iTrimFlexBits, 4);
    }
}

Void JxrEncoderPacketHeaderWriterWrite(
    CWMImageStrCodec* codec,
    CCodingContext* codingContext)
{
    JxrEncoderPacketHeaderPlan plan;
    U8 packetId;

    JxrEncoderPacketHeaderPlanInitialize(&plan,
        codec->WMISCP.bfBitstreamFormat,
        codec->cSB,
        codec->m_param.bTrimFlexbitsFlag,
        codec->m_bCtxLeft,
        codec->m_bCtxTop,
        codec->m_bSecondary,
        codec->m_param.bTranscode);
    if (!plan.writesHeaders)
        return;

    packetId = JxrEncoderPacketHeaderWriterGetPacketId(codec);
    if (plan.usesSpatialLayout)
        JxrEncoderPacketHeaderWriterWriteSpatial(codec, codingContext, &plan, packetId);
    else
        JxrEncoderPacketHeaderWriterWriteFrequency(codec, codingContext, &plan, packetId);
}
