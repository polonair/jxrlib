#include "JxrDecoderPacketPipeline.h"

#include "decode.h"
#include "JxrDecoderTileQuantizerSyntaxReader.h"
#include "JxrDecoderDcQuantizerHeaderApplier.h"
#include "JxrDecoderLpQuantizerHeaderApplier.h"
#include "JxrDecoderHpQuantizerHeaderApplier.h"
#include "JxrDecoderTileHeaderReader.h"
#include "JxrDecoderCodingContextResetter.h"
#include "JxrDecoderPacketRowReader.h"
#include "JxrPacketHeaderSyntaxReader.h"
static U8 JxrDecoderPacketPipelineReadQuantizer(U8 pQPIndex[MAX_CHANNELS], SimpleBitIO * pIO, size_t cChannel)
{
    JxrDecoderBitSource source;
    JxrDecoderQuantizerSyntax syntax;
    U8 cChMode;
    size_t channel;

    if(cChannel >= MAX_CHANNELS)
        return 0;
    JxrDecoderBitSourceInitSimple(&source, pIO);
    if (!JxrDecoderTileQuantizerSyntaxReadDc(&source, cChannel, &syntax))
        return 0;
    cChMode = syntax.channelMode;
    pQPIndex[0] = syntax.indices[0];
    if (cChMode == 1)
        pQPIndex[1] = syntax.indices[1];
    else if (cChMode > 0)
        for (channel = 1; channel < cChannel; ++channel)
            pQPIndex[channel] = syntax.indices[channel];
    return cChMode;
}

// packet header: 00000000 00000000 00000001 ?????xxx
// xxx:           000(spatial) 001(DC) 010(AD) 011(AC) 100(FL) 101-111(reserved)
// ?????:         (iTileY * cNumOfSliceV + iTileX) % 32
static Int JxrDecoderPacketPipelineReadHeader(BitIOInfo * pIO, U8 ptPacketType, U8 pID)
{
    JxrDecoderBitSource source;
    JxrPacketHeaderSyntax header;

    UNREFERENCED_PARAMETER(ptPacketType);
    UNREFERENCED_PARAMETER(pID);
    JxrDecoderBitSourceInitLegacy(&source, pIO);
    if (!JxrPacketHeaderSyntaxReaderRead(&source, &header) ||
        !JxrPacketHeaderSyntaxIsValid(&header))
    {
        return ICERR_ERROR;
    }
    return ICERR_OK;
}

static Int JxrDecoderPacketPipelineReadTileHeaderDc(CWMImageStrCodec * pSC, BitIOInfo * pIO)
{
    if((pSC->m_param.uQPMode & 1) != 0){
        JxrDecoderBitSource source;
        JxrDecoderQuantizerSyntax syntax;

        JxrDecoderBitSourceInitLegacy(&source, pIO);
        if(!JxrDecoderTileQuantizerSyntaxReadDc(&source, pSC->m_param.cNumChannels, &syntax) ||
            !JxrDecoderDcQuantizerHeaderApplierApply(pSC, &syntax))
            return ICERR_ERROR;
    }
    return ICERR_OK;
}
static Int JxrDecoderPacketPipelineReadTileHeaderLp(CWMImageStrCodec * pSC, BitIOInfo * pIO)
{
    if(pSC->WMISCP.sbSubband != SB_DC_ONLY && (pSC->m_param.uQPMode & 2) != 0){
        JxrDecoderBitSource source;
        JxrDecoderQuantizerSetSyntax syntax;

        JxrDecoderBitSourceInitLegacy(&source, pIO);
        if(!JxrDecoderTileQuantizerSyntaxReadLowpass(&source, pSC->m_param.cNumChannels, &syntax) ||
            !JxrDecoderLpQuantizerHeaderApplierApply(pSC, &syntax))
            return ICERR_ERROR;
    }
    return ICERR_OK;
}
static Int JxrDecoderPacketPipelineReadTileHeaderHp(CWMImageStrCodec * pSC, BitIOInfo * pIO)
{
    if(pSC->WMISCP.sbSubband != SB_DC_ONLY && pSC->WMISCP.sbSubband != SB_NO_HIGHPASS && (pSC->m_param.uQPMode & 4) != 0){
        JxrDecoderBitSource source;
        JxrDecoderQuantizerSetSyntax syntax;
        CWMITile * pTile = pSC->pTile + pSC->cTileColumn;

        JxrDecoderBitSourceInitLegacy(&source, pIO);
        if(!JxrDecoderTileQuantizerSyntaxReadHighpass(&source, pSC->m_param.cNumChannels, pTile->cNumQPLP, &syntax) ||
            !JxrDecoderHpQuantizerHeaderApplierApply(pSC, &syntax))
            return ICERR_ERROR;
    }
    return ICERR_OK;
}

static Void JxrDecoderTileHeaderReaderLegacyReadDc(Void* context,
    CWMImageStrCodec* codec, BitIOInfo* input)
{
    UNREFERENCED_PARAMETER(context);
    JxrDecoderPacketPipelineReadTileHeaderDc(codec, input);
}

static Void JxrDecoderTileHeaderReaderLegacyReadLp(Void* context,
    CWMImageStrCodec* codec, BitIOInfo* input)
{
    UNREFERENCED_PARAMETER(context);
    JxrDecoderPacketPipelineReadTileHeaderLp(codec, input);
}

static Void JxrDecoderTileHeaderReaderLegacyReadHp(Void* context,
    CWMImageStrCodec* codec, BitIOInfo* input)
{
    UNREFERENCED_PARAMETER(context);
    JxrDecoderPacketPipelineReadTileHeaderHp(codec, input);
}
static Void JxrDecoderCodingContextResetterLegacyReset(Void* context,
    CCodingContext* codingContext)
{
    UNREFERENCED_PARAMETER(context);
    ResetCodingContextDec(codingContext);
}

static Bool JxrDecoderResetCodingContextsLegacy(CWMImageStrCodec* codec, Bool resetAll)
{
    JxrDecoderCodingContextResetterConfig config;
    JxrDecoderCodingContextResetterOperations operations;

    config.contexts = codec->m_pCodingContext;
    config.contextCount = codec->WMISCP.cNumOfSliceMinus1V + 1;
    config.resetAll = resetAll;
    operations.context = NULL;
    operations.reset = JxrDecoderCodingContextResetterLegacyReset;
    return JxrDecoderCodingContextResetterResetContexts(&config, &operations);
}

static Bool JxrDecoderPacketAttachmentLegacyDetach(Void* context, BitIOInfo* reader)
{
    return detachISRead((CWMImageStrCodec*)context, reader) == WMP_errSuccess;
}

static Bool JxrDecoderPacketAttachmentLegacyAttach(Void* context, BitIOInfo* reader,
    struct WMPStream* stream)
{
    return attachISRead(reader, stream, (CWMImageStrCodec*)context) == WMP_errSuccess;
}

static Bool JxrDecoderPacketAttachmentLegacySeek(Void* context, struct WMPStream* stream,
    U64 offset)
{
    UNREFERENCED_PARAMETER(context);
    return stream->SetPos(stream, (size_t)offset) == WMP_errSuccess;
}
static Bool JxrDecoderPacketHeaderReaderLegacyRead(Void* context, BitIOInfo* reader,
    U8 packetType, U8 packetId)
{
    UNREFERENCED_PARAMETER(context);
    return JxrDecoderPacketPipelineReadHeader(reader, packetType, packetId) == ICERR_OK;
}

static Bool JxrDecoderPacketHeaderReaderLegacyReadTrim(Void* context, BitIOInfo* reader,
    U32* value)
{
    UNREFERENCED_PARAMETER(context);
    *value = getBit16(reader, 4);
    return TRUE;
}

static Bool JxrDecoderPacketHeaderReaderLegacyStoreTrim(Void* context, U32 tileColumn,
    Int value)
{
    ((CWMImageStrCodec*)context)->m_pCodingContext[tileColumn].m_iTrimFlexBits = value;
    return TRUE;
}
Int JxrDecoderPacketPipelineReadCurrentMacroblock(CWMImageStrCodec* pSC)
{
    if (pSC->cColumn == 0 && pSC->cRow == pSC->WMISCP.uiTileY[pSC->cTileRow]) {
        if (pSC->m_bSecondary) {
            if (!JxrDecoderResetCodingContextsLegacy(pSC, pSC->cNumBitIO > 0))
                return ICERR_ERROR;
        }
        else {
            JxrDecoderPacketRowReaderOperations packetRowOperations;

            packetRowOperations.attachment.context = pSC;
            packetRowOperations.attachment.detach = JxrDecoderPacketAttachmentLegacyDetach;
            packetRowOperations.attachment.attach = JxrDecoderPacketAttachmentLegacyAttach;
            packetRowOperations.attachment.seek = JxrDecoderPacketAttachmentLegacySeek;
            packetRowOperations.header.context = pSC;
            packetRowOperations.header.readHeader = JxrDecoderPacketHeaderReaderLegacyRead;
            packetRowOperations.header.readTrim = JxrDecoderPacketHeaderReaderLegacyReadTrim;
            packetRowOperations.header.storeTrim = JxrDecoderPacketHeaderReaderLegacyStoreTrim;
            packetRowOperations.resetter.context = NULL;
            packetRowOperations.resetter.reset = JxrDecoderCodingContextResetterLegacyReset;
            if (!JxrDecoderPacketRowReaderRead(pSC, &packetRowOperations))
                return ICERR_ERROR;
        }
    }

    if (pSC->m_bCtxLeft && pSC->m_bCtxTop && pSC->m_bSecondary == FALSE) {
        CCodingContext* pContext = &pSC->m_pCodingContext[pSC->cTileColumn];
        JxrDecoderTileHeaderReaderConfig tileHeaders;
        JxrDecoderTileHeaderReaderOperations tileHeaderOperations;

        tileHeaders.primaryCodec = pSC;
        tileHeaders.secondaryCodec = pSC->m_pNextSC;
        tileHeaders.dcInput = pContext->m_pIODC;
        tileHeaders.lpInput = pContext->m_pIOLP;
        tileHeaders.hpInput = pContext->m_pIOAC;
        tileHeaders.subbandCount = (U8)pSC->cSB;
        tileHeaderOperations.context = NULL;
        tileHeaderOperations.readDc = JxrDecoderTileHeaderReaderLegacyReadDc;
        tileHeaderOperations.readLp = JxrDecoderTileHeaderReaderLegacyReadLp;
        tileHeaderOperations.readHp = JxrDecoderTileHeaderReaderLegacyReadHp;
        if (!JxrDecoderTileHeaderReaderRead(&tileHeaders, &tileHeaderOperations))
            return ICERR_ERROR;
    }

    return ICERR_OK;
}
