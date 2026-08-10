#include "JxrDcDecoder.h"
#include "decode.h"
#include "JxrEntropyLevelDecoder.h"
#include "JxrEntropyReader.h"
#include "JxrAdaptiveHuffman.h"
#include "JxrHuffmanDecoder.h"
#include "JxrQuantizationIndexReader.h"
#include "JxrSubbandStreamRefill.h"

Int JxrDcDecoderDecodeSubband(JxrDecoderSubbandContext* state, Int macroblockX, Int macroblockY)
{
    CWMImageStrCodec* codec = state->codec;
    CWMITile* tile = codec->pTile + codec->cTileColumn;
    CWMIMBInfo* macroblock = &codec->MBInfo;
    const COLORFORMAT colorFormat = codec->m_param.cfColorFormat;
    const Int channelCount = (Int)codec->m_param.cNumChannels;
    JxrEntropyBitReader* reader = &state->dcReader;
    Int index;
    Int channel;
    Int laplacianMean[2] = { 0, 0 };
    Int* currentMean = laplacianMean;
    Int modelBits = JxrAdaptiveModelStateGetFlcBits(&state->dcModelState, 0);
    CAdaptiveHuffman* significantFlags;
    Int luminance;
    Int chromaU;
    Int chromaV;

    UNREFERENCED_PARAMETER(macroblockX);
    UNREFERENCED_PARAMETER(macroblockY);

    for (channel = 0; channel < channelCount; ++channel) {
        memset(macroblock->iBlockDC[channel], 0, 16 * sizeof(I32));
    }

    JxrSubbandStreamRefillLevel1(codec, reader);
    macroblock->iQIndexLP = 0;
    macroblock->iQIndexHP = 0;

    if (codec->WMISCP.bfBitstreamFormat == SPATIAL && codec->WMISCP.sbSubband != SB_DC_ONLY) {
        if (tile->cBitsLP > 0) macroblock->iQIndexLP = JxrQuantizationIndexReaderDecode(reader, tile->cBitsLP);
        if (codec->WMISCP.sbSubband != SB_NO_HIGHPASS && tile->cBitsHP > 0) {
            macroblock->iQIndexHP = JxrQuantizationIndexReaderDecode(reader, tile->cBitsHP);
        }
    }
    if (tile->cBitsHP == 0 && tile->cNumQPHP > 1) macroblock->iQIndexHP = macroblock->iQIndexLP;
    if (macroblock->iQIndexLP >= tile->cNumQPLP || macroblock->iQIndexHP >= tile->cNumQPHP) return ICERR_ERROR;

    if (colorFormat == Y_ONLY || colorFormat == CMYK || colorFormat == NCOMPONENT) {
        for (channel = 0; channel < channelCount; ++channel) {
            luminance = 0;
            if (JxrEntropyBitReaderReadFlag(reader)) {
                luminance = JxrEntropyLevelDecoderDecodeReader(
                    JxrHuffmanStateSetGet(&state->huffmanStateSet, 3), reader) - 1;
                *currentMean += 1;
            }
            if (modelBits) luminance = (luminance << modelBits) | (Int)JxrEntropyBitReaderRead(reader, modelBits);
            if (luminance && JxrEntropyBitReaderReadFlag(reader)) luminance = -luminance;
            macroblock->iBlockDC[channel][0] = luminance;
            currentMean = laplacianMean + 1;
            modelBits = JxrAdaptiveModelStateGetFlcBits(&state->dcModelState, 1);
        }
    }
    else {
        significantFlags = JxrHuffmanStateSetGet(&state->huffmanStateSet, 2);
        {
            JxrHuffmanTable table = JxrHuffmanTableCreate(significantFlags->m_hufDecTable);
            index = JxrHuffmanDecoderDecodeSymbolReader(&table, reader);
        }
        luminance = index >> 2;
        chromaU = (index >> 1) & 1;
        chromaV = index & 1;

        if (luminance) { luminance = JxrEntropyLevelDecoderDecodeReader(JxrHuffmanStateSetGet(&state->huffmanStateSet, 3), reader) - 1; *currentMean += 1; }
        if (modelBits) luminance = (luminance << modelBits) | (Int)JxrEntropyBitReaderRead(reader, modelBits);
        if (luminance && JxrEntropyBitReaderReadFlag(reader)) luminance = -luminance;
        macroblock->iBlockDC[0][0] = luminance;

        currentMean = laplacianMean + 1;
        modelBits = JxrAdaptiveModelStateGetFlcBits(&state->dcModelState, 1);
        if (chromaU) { chromaU = JxrEntropyLevelDecoderDecodeReader(JxrHuffmanStateSetGet(&state->huffmanStateSet, 4), reader) - 1; *currentMean += 1; }
        if (modelBits) chromaU = (chromaU << modelBits) | (Int)JxrEntropyBitReaderRead(reader, modelBits);
        if (chromaU && JxrEntropyBitReaderReadFlag(reader)) chromaU = -chromaU;
        macroblock->iBlockDC[1][0] = chromaU;

        if (chromaV) { chromaV = JxrEntropyLevelDecoderDecodeReader(JxrHuffmanStateSetGet(&state->huffmanStateSet, 4), reader) - 1; *currentMean += 1; }
        if (modelBits) chromaV = (chromaV << modelBits) | (Int)JxrEntropyBitReaderRead(reader, modelBits);
        if (chromaV && JxrEntropyBitReaderReadFlag(reader)) chromaV = -chromaV;
        macroblock->iBlockDC[2][0] = chromaV;
    }

    JxrAdaptiveModelStateUpdateForMacroblock(&state->dcModelState, colorFormat,
        channelCount, laplacianMean);
    if (((!(codec->WMISCP.bfBitstreamFormat != FREQUENCY || codec->m_Dparam->cThumbnailScale < 16)) ||
        codec->WMISCP.sbSubband == SB_DC_ONLY) && codec->m_bResetContext) {
        Int tableIndex;
        for (tableIndex = 2; tableIndex < 5; ++tableIndex) {
            JxrHuffmanStateSetAdapt(&state->huffmanStateSet, tableIndex);
        }
    }
    return ICERR_OK;
}

Int JxrDcDecoderDecodeMacroblock(CWMImageStrCodec* codec, CCodingContext* entropy,
    Int macroblockX, Int macroblockY)
{
    JxrDecoderSubbandContext state;
    JxrDecoderSubbandContextInit(&state, codec, entropy);
    return JxrDcDecoderDecodeSubband(&state, macroblockX, macroblockY);
}
