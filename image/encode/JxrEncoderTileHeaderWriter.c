#include "JxrEncoderTileHeaderWriter.h"

Void JxrEncoderTileHeaderPlanInitialize(
    JxrEncoderTileHeaderPlan* plan,
    U32 quantizerMode,
    SUBBAND subband)
{
    plan->writesDcQuantizer = (quantizerMode & 1) != 0;
    plan->writesLpQuantizer = subband != SB_DC_ONLY && (quantizerMode & 2) != 0;
    plan->writesHpQuantizer = subband != SB_DC_ONLY &&
        subband != SB_NO_HIGHPASS && (quantizerMode & 4) != 0;
}

Void JxrEncoderTileHeaderWriterWriteQuantizer(
    CWMIQuantizer* quantizers[MAX_CHANNELS],
    BitIOInfo* bitWriter,
    U8 channelMode,
    size_t channelCount,
    size_t quantizerIndex)
{
    size_t channelIndex;

    if (channelMode > 2)
        channelMode = 2;
    if (channelCount > 1)
        putBit16(bitWriter, channelMode, 2);
    else
        channelMode = 0;

    putBit16(bitWriter, quantizers[0][quantizerIndex].iIndex, 8);
    if (channelMode == 1)
        putBit16(bitWriter, quantizers[1][quantizerIndex].iIndex, 8);
    else if (channelMode > 0)
        for (channelIndex = 1; channelIndex < channelCount; ++channelIndex)
            putBit16(bitWriter, quantizers[channelIndex][quantizerIndex].iIndex, 8);
}

Int JxrEncoderTileHeaderWriterWriteDc(
    CWMImageStrCodec* codec,
    BitIOInfo* bitWriter)
{
    CWMImageStrCodec* currentCodec = codec;
    size_t codecCount = codec->m_pNextSC == NULL ? 1U : 2U;
    size_t tileIndex;

    for (; codecCount > 0; --codecCount) {
        JxrEncoderTileHeaderPlan plan;

        JxrEncoderTileHeaderPlanInitialize(&plan,
            currentCodec->m_param.uQPMode, currentCodec->WMISCP.sbSubband);
        if (plan.writesDcQuantizer) {
            CWMITile* tile = currentCodec->pTile + currentCodec->cTileColumn;
            size_t channelIndex;

            tile->cChModeDC = (U8)(rand() & 3);
            if (currentCodec->cTileRow + currentCodec->cTileColumn == 0)
                for (tileIndex = 0;
                    tileIndex <= currentCodec->WMISCP.cNumOfSliceMinus1V;
                    ++tileIndex)
                    if (allocateQuantizer(currentCodec->pTile[tileIndex].pQuantizerDC,
                        currentCodec->m_param.cNumChannels, 1) != ICERR_OK)
                        return ICERR_ERROR;

            for (channelIndex = 0;
                channelIndex < currentCodec->m_param.cNumChannels;
                ++channelIndex)
                tile->pQuantizerDC[channelIndex]->iIndex = (U8)((rand() & 0x2f) + 1);

            formatQuantizer(tile->pQuantizerDC, tile->cChModeDC,
                currentCodec->m_param.cNumChannels, 0, TRUE,
                currentCodec->m_param.bScaledArith);
            for (channelIndex = 0;
                channelIndex < currentCodec->m_param.cNumChannels;
                ++channelIndex)
                tile->pQuantizerDC[channelIndex]->iOffset =
                    tile->pQuantizerDC[channelIndex]->iQP >> 1;

            JxrEncoderTileHeaderWriterWriteQuantizer(tile->pQuantizerDC,
                bitWriter, tile->cChModeDC, currentCodec->m_param.cNumChannels, 0);
        }
        currentCodec = currentCodec->m_pNextSC;
    }
    return ICERR_OK;
}

Int JxrEncoderTileHeaderWriterWriteLp(
    CWMImageStrCodec* codec,
    BitIOInfo* bitWriter)
{
    CWMImageStrCodec* currentCodec = codec;
    size_t codecCount = codec->m_pNextSC == NULL ? 1U : 2U;

    for (; codecCount > 0; --codecCount) {
        JxrEncoderTileHeaderPlan plan;

        JxrEncoderTileHeaderPlanInitialize(&plan,
            currentCodec->m_param.uQPMode, currentCodec->WMISCP.sbSubband);
        if (plan.writesLpQuantizer) {
            CWMITile* tile = currentCodec->pTile + currentCodec->cTileColumn;
            U8 quantizerIndex;
            U8 channelIndex;

            tile->bUseDC = (rand() & 1) == 0 ? TRUE : FALSE;
            putBit16(bitWriter, tile->bUseDC == TRUE ? 1 : 0, 1);
            tile->cBitsLP = 0;
            tile->cNumQPLP = tile->bUseDC == TRUE ? 1 : (U8)((rand() & 0xf) + 1);
            if (currentCodec->cTileRow > 0)
                freeQuantizer(tile->pQuantizerLP);
            if (allocateQuantizer(tile->pQuantizerLP,
                currentCodec->m_param.cNumChannels, tile->cNumQPLP) != ICERR_OK)
                return ICERR_ERROR;

            if (tile->bUseDC == TRUE)
                useDCQuantizer(currentCodec, currentCodec->cTileColumn);
            else {
                putBit16(bitWriter, tile->cNumQPLP - 1, 4);
                tile->cBitsLP = dquantBits(tile->cNumQPLP);
                for (quantizerIndex = 0; quantizerIndex < tile->cNumQPLP; ++quantizerIndex) {
                    tile->cChModeLP[quantizerIndex] = (U8)(rand() & 3);
                    for (channelIndex = 0;
                        channelIndex < currentCodec->m_param.cNumChannels;
                        ++channelIndex)
                        tile->pQuantizerLP[channelIndex][quantizerIndex].iIndex =
                            (U8)((rand() & 0xfe) + 1);
                    formatQuantizer(tile->pQuantizerLP,
                        tile->cChModeLP[quantizerIndex], currentCodec->m_param.cNumChannels,
                        quantizerIndex, TRUE, currentCodec->m_param.bScaledArith);
                    JxrEncoderTileHeaderWriterWriteQuantizer(tile->pQuantizerLP,
                        bitWriter, tile->cChModeLP[quantizerIndex],
                        currentCodec->m_param.cNumChannels, quantizerIndex);
                }
            }
        }
        currentCodec = currentCodec->m_pNextSC;
    }
    return ICERR_OK;
}

Int JxrEncoderTileHeaderWriterWriteHp(
    CWMImageStrCodec* codec,
    BitIOInfo* bitWriter)
{
    CWMImageStrCodec* currentCodec = codec;
    size_t codecCount = codec->m_pNextSC == NULL ? 1U : 2U;

    for (; codecCount > 0; --codecCount) {
        JxrEncoderTileHeaderPlan plan;

        JxrEncoderTileHeaderPlanInitialize(&plan,
            currentCodec->m_param.uQPMode, currentCodec->WMISCP.sbSubband);
        if (plan.writesHpQuantizer) {
            CWMITile* tile = currentCodec->pTile + currentCodec->cTileColumn;
            U8 quantizerIndex;
            U8 channelIndex;

            tile->bUseLP = (rand() & 1) == 0 ? TRUE : FALSE;
            putBit16(bitWriter, tile->bUseLP == TRUE ? 1 : 0, 1);
            tile->cBitsHP = 0;
            tile->cNumQPHP = tile->bUseLP == TRUE ? tile->cNumQPLP :
                (U8)((rand() & 0xf) + 1);
            if (currentCodec->cTileRow > 0)
                freeQuantizer(tile->pQuantizerHP);
            if (allocateQuantizer(tile->pQuantizerHP,
                currentCodec->m_param.cNumChannels, tile->cNumQPHP) != ICERR_OK)
                return ICERR_ERROR;

            if (tile->bUseLP == TRUE)
                useLPQuantizer(currentCodec, tile->cNumQPHP, currentCodec->cTileColumn);
            else {
                putBit16(bitWriter, tile->cNumQPHP - 1, 4);
                tile->cBitsHP = dquantBits(tile->cNumQPHP);
                for (quantizerIndex = 0; quantizerIndex < tile->cNumQPHP; ++quantizerIndex) {
                    tile->cChModeHP[quantizerIndex] = (U8)(rand() & 3);
                    for (channelIndex = 0;
                        channelIndex < currentCodec->m_param.cNumChannels;
                        ++channelIndex)
                        tile->pQuantizerHP[channelIndex][quantizerIndex].iIndex =
                            (U8)((rand() & 0xfe) + 1);
                    formatQuantizer(tile->pQuantizerHP,
                        tile->cChModeHP[quantizerIndex], currentCodec->m_param.cNumChannels,
                        quantizerIndex, FALSE, currentCodec->m_param.bScaledArith);
                    JxrEncoderTileHeaderWriterWriteQuantizer(tile->pQuantizerHP,
                        bitWriter, tile->cChModeHP[quantizerIndex],
                        currentCodec->m_param.cNumChannels, quantizerIndex);
                }
            }
        }
        currentCodec = currentCodec->m_pNextSC;
    }
    return ICERR_OK;
}
