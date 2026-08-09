#include "JxrDcDecoder.h"
#include "decode.h"
#include "JxrEntropyLevelDecoder.h"
#include "JxrEntropyReader.h"
#include "JxrAdaptiveHuffman.h"
#include "JxrHuffmanDecoder.h"

U8 decodeQPIndex(BitIOInfo* input, U8 bitCount);

Int JxrDcDecoderDecodeMacroblock(CWMImageStrCodec* codec, CCodingContext* context,
    Int macroblockX, Int macroblockY)
{
    CWMITile* tile = codec->pTile + codec->cTileColumn;
    CWMIMBInfo* macroblock = &codec->MBInfo;
    const COLORFORMAT colorFormat = codec->m_param.cfColorFormat;
    const Int channelCount = (Int)codec->m_param.cNumChannels;
    BitIOInfo* input = context->m_pIODC;
    Int index;
    Int channel;
    Int laplacianMean[2] = { 0, 0 };
    Int* currentMean = laplacianMean;
    Int modelBits = context->m_aModelDC.m_iFlcBits[0];
    CAdaptiveHuffman* significantFlags;
    Int luminance;
    Int chromaU;
    Int chromaV;

    UNREFERENCED_PARAMETER(macroblockX);
    UNREFERENCED_PARAMETER(macroblockY);

    for (channel = 0; channel < channelCount; ++channel) {
        memset(macroblock->iBlockDC[channel], 0, 16 * sizeof(I32));
    }

    readIS_L1(codec, input);
    macroblock->iQIndexLP = 0;
    macroblock->iQIndexHP = 0;

    if (codec->WMISCP.bfBitstreamFormat == SPATIAL && codec->WMISCP.sbSubband != SB_DC_ONLY) {
        if (tile->cBitsLP > 0) macroblock->iQIndexLP = decodeQPIndex(input, tile->cBitsLP);
        if (codec->WMISCP.sbSubband != SB_NO_HIGHPASS && tile->cBitsHP > 0) {
            macroblock->iQIndexHP = decodeQPIndex(input, tile->cBitsHP);
        }
    }
    if (tile->cBitsHP == 0 && tile->cNumQPHP > 1) macroblock->iQIndexHP = macroblock->iQIndexLP;
    if (macroblock->iQIndexLP >= tile->cNumQPLP || macroblock->iQIndexHP >= tile->cNumQPHP) return ICERR_ERROR;

    if (colorFormat == Y_ONLY || colorFormat == CMYK || colorFormat == NCOMPONENT) {
        for (channel = 0; channel < channelCount; ++channel) {
            luminance = 0;
            if (JxrEntropyReaderReadFlag(input)) {
                luminance = JxrEntropyLevelDecoderDecode(context->m_pAHexpt[3], input) - 1;
                *currentMean += 1;
            }
            if (modelBits) luminance = (luminance << modelBits) | (Int)JxrEntropyReaderRead(input, modelBits);
            if (luminance && JxrEntropyReaderReadFlag(input)) luminance = -luminance;
            macroblock->iBlockDC[channel][0] = luminance;
            currentMean = laplacianMean + 1;
            modelBits = context->m_aModelDC.m_iFlcBits[1];
        }
    }
    else {
        significantFlags = context->m_pAHexpt[2];
        {
            JxrHuffmanTable table = JxrHuffmanTableCreate(significantFlags->m_hufDecTable);
            index = JxrHuffmanDecoderDecodeSymbol(&table, input);
        }
        luminance = index >> 2;
        chromaU = (index >> 1) & 1;
        chromaV = index & 1;

        if (luminance) { luminance = JxrEntropyLevelDecoderDecode(context->m_pAHexpt[3], input) - 1; *currentMean += 1; }
        if (modelBits) luminance = (luminance << modelBits) | (Int)JxrEntropyReaderRead(input, modelBits);
        if (luminance && JxrEntropyReaderReadFlag(input)) luminance = -luminance;
        macroblock->iBlockDC[0][0] = luminance;

        currentMean = laplacianMean + 1;
        modelBits = context->m_aModelDC.m_iFlcBits[1];
        if (chromaU) { chromaU = JxrEntropyLevelDecoderDecode(context->m_pAHexpt[4], input) - 1; *currentMean += 1; }
        if (modelBits) chromaU = (chromaU << modelBits) | (Int)JxrEntropyReaderRead(input, modelBits);
        if (chromaU && JxrEntropyReaderReadFlag(input)) chromaU = -chromaU;
        macroblock->iBlockDC[1][0] = chromaU;

        if (chromaV) { chromaV = JxrEntropyLevelDecoderDecode(context->m_pAHexpt[4], input) - 1; *currentMean += 1; }
        if (modelBits) chromaV = (chromaV << modelBits) | (Int)JxrEntropyReaderRead(input, modelBits);
        if (chromaV && JxrEntropyReaderReadFlag(input)) chromaV = -chromaV;
        macroblock->iBlockDC[2][0] = chromaV;
    }

    UpdateModelMB(colorFormat, channelCount, laplacianMean, &context->m_aModelDC);
    if (((!(codec->WMISCP.bfBitstreamFormat != FREQUENCY || codec->m_Dparam->cThumbnailScale < 16)) ||
        codec->WMISCP.sbSubband == SB_DC_ONLY) && codec->m_bResetContext) {
        Int tableIndex;
        for (tableIndex = 2; tableIndex < 5; ++tableIndex) {
            JxrAdaptiveHuffmanAdapt(context->m_pAHexpt[tableIndex]);
        }
    }
    return ICERR_OK;
}
