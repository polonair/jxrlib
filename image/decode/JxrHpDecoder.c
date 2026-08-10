#include "JxrHpDecoder.h"
#include "decode.h"
#include "JxrAdaptiveHuffman.h"
#include "JxrAdaptiveScan.h"
#include "JxrCoefficientBuffer.h"
#include "JxrCbpPredictor.h"
#include "JxrHpCoefficientBlockResolver.h"
#include "JxrEntropyBlockDecoder.h"
#include "JxrHpBlockDecoder.h"
#include "JxrEntropyLevelDecoder.h"
#include "JxrEntropyReader.h"

extern const int dctIndex[3][16];
U8 decodeQPIndex(BitIOInfo* input, U8 bitCount);

/*************************************************************************
    DecodeCBP
*************************************************************************/
Void JxrHpDecoderDecodeCbp(JxrDecoderSubbandContext* state)
{
    CWMImageStrCodec* codec = state->codec;
    BitIOInfo* pIO = state->highpassInput;
    const COLORFORMAT cf = codec->m_param.cfColorFormat;
    const Int iChannel = (cf == NCOMPONENT || cf == CMYK) ? (Int) codec->m_param.cNumChannels : 1;
    Int iCBPCY, iCBPCU , iCBPCV;
    Int k, iBlock, i;
    Int iNumCBP;
    Bool bIsChroma;
    CAdaptiveHuffman* pAHCBP = state->cbpHuffman;
    CAdaptiveHuffman* pAHCBP1 = state->cbpCountHuffman;
    CAdaptiveHuffman* pAHex1 = state->huffmanStates[1];

    readIS_L1(codec, pIO);

    for (i = 0; i < iChannel; i++) {

        iCBPCY = iCBPCU = iCBPCV = 0;
        iNumCBP = JxrAdaptiveHuffmanDecodeShortTable(pAHCBP1->m_hufDecTable, pIO);
        JxrAdaptiveHuffmanObserve(pAHCBP1, iNumCBP);

        switch (iNumCBP) {
            case 2:
                iNumCBP = JxrEntropyReaderRead(pIO, 2);
                if (iNumCBP == 0)
                    iNumCBP = 3;
                else if (iNumCBP == 1)
                    iNumCBP = 5;
                else {
                    static const Int aTab[] = { 6, 9, 10, 12 };
                    iNumCBP = aTab[iNumCBP * 2 + JxrEntropyReaderReadFlag (pIO) - 4];
                }
                break;
            case 1:
                iNumCBP = 1 << JxrEntropyReaderRead(pIO, 2);
                break;
            case 3:
                iNumCBP = 0xf ^ (1 << JxrEntropyReaderRead(pIO, 2));
                break;
            case 4:
                iNumCBP = 0xf;
        }

        for (iBlock = 0; iBlock < 4; iBlock++) {
            if (iNumCBP & (1 << iBlock)) {
                static const UInt gFLC0[] = { 0,2,1,2,2,0 };
                static const UInt gOff0[] = { 0,4,2,8,12,1 };
                static const UInt gOut0[] = { 0,15,3,12, 1,2,4,8, 5,6,9,10, 7,11,13,14 };
                Int iNumBlockCBP = JxrAdaptiveHuffmanDecode(pAHCBP, pIO);
                unsigned int val = (unsigned int) iNumBlockCBP + 1, iCode1;

                /* Observation is performed by JxrAdaptiveHuffmanDecode. */
                iNumBlockCBP = 0;

                if (val >= 6) { // chroma present
                    if (JxrEntropyReaderReadFlag (pIO)) {
                        iNumBlockCBP = 0x10;
                    }
                    else if (JxrEntropyReaderReadFlag (pIO)) {
                        iNumBlockCBP = 0x20;
                    }
                    else {
                        iNumBlockCBP = 0x30;
                    }
                    if (val == 9) {
                        if (JxrEntropyReaderReadFlag (pIO)) {
                            // do nothing
                        }
                        else if (JxrEntropyReaderReadFlag (pIO)) {
                            val = 10;
                        }
                        else {
                            val = 11;
                        }
                    }
                    val -= 6;
                }
                iCode1 = gOff0[val];
                if (gFLC0[val]) {
                    iCode1 += JxrEntropyReaderRead(pIO, gFLC0[val]);
                }
                iNumBlockCBP += gOut0[iCode1];

                switch (cf) {
                    case YUV_444:
                        iCBPCY |= ((iNumBlockCBP & 0xf) << (iBlock * 4));
                        for (k = 0; k < 2; k++) {
                            bIsChroma = ((iNumBlockCBP>>(k+4)) & 0x01);
                            if (bIsChroma) { // U is present in block
                                Int iCode = JxrAdaptiveHuffmanDecodeShortTable(pAHex1->m_hufDecTable, pIO);
                                switch (iCode) {
                            case 1:
                                iCode = JxrEntropyReaderRead(pIO, 2);
                                if (iCode == 0)
                                    iCode = 3;
                                else if (iCode == 1)
                                    iCode = 5;
                                else {
                                    static const Int aTab[] = { 6, 9, 10, 12 };
                                    iCode = aTab[iCode * 2 + JxrEntropyReaderReadFlag (pIO) - 4];
                                }
                                break;
                            case 0:
                                iCode = 1 << JxrEntropyReaderRead(pIO, 2);
                                break;
                            case 2:
                                iCode = 0xf ^ (1 << JxrEntropyReaderRead(pIO, 2));
                                break;
                            case 3:
                                iCode = 0xf;
                                }
                                if (k == 0)
                                    iCBPCU |= (iCode << (iBlock * 4));
                                else
                                    iCBPCV |= (iCode << (iBlock * 4));
                            }
                        }
                        break;

                    case YUV_420:
                        iCBPCY |= ((iNumBlockCBP & 0xf) << (iBlock * 4));
                        iCBPCU |= ((iNumBlockCBP >> 4) & 0x1) << (iBlock);
                        iCBPCV |= ((iNumBlockCBP >> 5) & 0x1) << (iBlock);
                        break;

                    case YUV_422:
                        iCBPCY |= ((iNumBlockCBP & 0xf) << (iBlock * 4));
                        for (k = 0; k < 2; k ++) {
                            Int iCode = 5;
                            const Int iShift[4] = {0, 1, 4, 5};
                            if((iNumBlockCBP >> (k + 4)) & 0x01) {
                                if(JxrEntropyReaderReadFlag(pIO)) {
                                    iCode = 1;
                                }
                                else if(JxrEntropyReaderReadFlag(pIO)){
                                    iCode = 4;
                                }
                                iCode <<= iShift[iBlock];
                                if(k == 0) iCBPCU |= iCode;
                                else iCBPCV |= iCode;
                            }
                        }
                        break;

                    default:
                        iCBPCY |= (iNumBlockCBP << (iBlock * 4));
                }
            }
        }

        state->differentialCbp[i] = iCBPCY;
        if (cf == YUV_420 || cf == YUV_444 || cf == YUV_422) {
            state->differentialCbp[1] = iCBPCU;
            state->differentialCbp[2] = iCBPCV;
        }
    }
}

/*************************************************************************
    GetCoeffs
*************************************************************************/
static Int JxrHpDecoderDecodeCoefficients(JxrDecoderSubbandContext* state,
    Int macroblockX, Int macroblockY)
{
    CWMImageStrCodec* codec = state->codec;
    CWMITile* pTile = codec->pTile + codec->cTileColumn;
    BitIOInfo* pIO = state->highpassInput;
    BitIOInfo* pIOFL = state->flexbitsInput;
    JxrEntropyBitReader highpassReader;
    JxrEntropyBitReader flexbitsReader;
    const COLORFORMAT cf = codec->m_param.cfColorFormat;
    const Int iChannels = (Int) codec->m_param.cNumChannels;
    const Int iPlanes = (cf == YUV_420 || cf == YUV_422) ? 1 : iChannels;
    Int  iQP;
    CAdaptiveScan *pScan;
    JxrCoefficientBuffer coefficients;
    Int i, iBlock, iSubblock, iNBlocks = 4;
    Int iModelBits = state->highpassModel->m_iFlcBits[0];
    Int aLaplacianMean[2] = { 0, 0}, *pLM = aLaplacianMean + 0;
    const Int *pOrder = dctIndex[0];
    const Int iOrient = codec->MBInfo.iOrientation;
    Bool bChroma = FALSE;

    Int iCBPCU = state->cbp[1];
    Int iCBPCV = state->cbp[2];
    Int iCBPCY = state->cbp[0];

    UNREFERENCED_PARAMETER(macroblockX);
    UNREFERENCED_PARAMETER(macroblockY);
    JxrEntropyBitReaderInit(&highpassReader, pIO);
    JxrEntropyBitReaderInit(&flexbitsReader, pIOFL);

    /** set scan arrays and other MB level constants **/
    if (iOrient == 1) {
        pScan = state->verticalScan;
    }
    else {
        pScan = state->horizontalScan;
    }

    if (cf == YUV_420) {
        iNBlocks = 6;
        iCBPCY += (iCBPCU << 16) + (iCBPCV << 20);
    }
    else if (cf == YUV_422) {
        iNBlocks = 8;
        iCBPCY += (iCBPCU << 16) + (iCBPCV << 24);
    }

    for (i = 0; i < iPlanes; i++) {
        Int iIndex = 0, iNumNonZero;

        if(codec->WMISCP.sbSubband != SB_NO_FLEXBITS)
            readIS_L1(codec, pIOFL);

        for (iBlock = 0; iBlock < iNBlocks; iBlock++) {

            readIS_L2(codec, pIO);
            if (pIO != pIOFL)
                readIS_L2(codec, pIOFL);

            iQP = (codec->m_param.bTranscode ? 1 : pTile->pQuantizerHP[iPlanes > 1 ? i : (iBlock > 3 ? (cf == YUV_420 ? iBlock - 3 : iBlock / 2 - 1) : 0)][codec->MBInfo.iQIndexHP].iQP);

            for (iSubblock = 0; iSubblock < 4; iSubblock++, iIndex++, iCBPCY >>= 1) {
                coefficients = JxrHpCoefficientBlockResolverResolve(state, i, iBlock,
                    iSubblock, iIndex);

                /** read AC values **/
                assert (codec->m_Dparam->bSkipFlexbits == 0 || codec->WMISCP.bfBitstreamFormat == FREQUENCY || codec->WMISCP.sbSubband == SB_NO_FLEXBITS);
                {
                    JxrHpBlockDecodingContext blockContext;
                    blockContext.huffmanStates = state->huffmanStates;
                    blockContext.highpassReader = &highpassReader;
                    blockContext.flexbitsReader = &flexbitsReader;
                    blockContext.coefficientBuffer = &coefficients;
                    blockContext.scan = pScan;
                    blockContext.coefficientOrder = pOrder;
                    blockContext.isChroma = bChroma;
                    blockContext.hasCoefficients = (iCBPCY & 1) != 0;
                    blockContext.skipFlexbits = codec->m_Dparam->bSkipFlexbits;
                    blockContext.modelBits = iModelBits;
                    blockContext.trimFlexBits = state->trimFlexBits;
                    blockContext.quantizationParameter = iQP;
                    iNumNonZero = JxrHpBlockDecoderDecode(&blockContext);
                }
                if(iNumNonZero > 16) // something is wrong!
                    return ICERR_ERROR;
                // shouldn't this be > 15?
                (*pLM) += iNumNonZero;
            }
            if (iBlock == 3) {
                iModelBits = state->highpassModel->m_iFlcBits[1];
                pLM = aLaplacianMean + 1;
                bChroma = TRUE;
            }
        }

        iCBPCY = state->cbp[(i + 1) & 0xf];
        assert (MAX_CHANNELS == 16);
    }

    /** update model at end of MB **/
    UpdateModelMB(cf, iChannels, aLaplacianMean, state->highpassModel);
    return ICERR_OK;
}


Int JxrHpDecoderDecodeSubband(JxrDecoderSubbandContext* state,
    Int macroblockX, Int macroblockY)
{
    CWMImageStrCodec* codec = state->codec;
    CWMITile* tile = codec->pTile + codec->cTileColumn;
    Int tableIndex;

    /** reset adaptive scan totals **/
    if (codec->m_bResetRGITotals) {
        JxrAdaptiveScanResetTotals(state->horizontalScan, 16);
        JxrAdaptiveScanResetTotals(state->verticalScan, 16);
    }
    if((codec->WMISCP.bfBitstreamFormat != SPATIAL) && (tile->cBitsHP > 0)) { // MB-based HP QP index
        codec->MBInfo.iQIndexHP = decodeQPIndex(state->highpassInput, tile->cBitsHP);
        if (codec->MBInfo.iQIndexHP >= tile->cNumQPHP)
            goto ErrorExit;
    }
    else if(tile->cBitsHP == 0 && tile->cNumQPHP > 1) // use LP QP
        codec->MBInfo.iQIndexHP = codec->MBInfo.iQIndexLP;


    JxrHpDecoderDecodeCbp(state);
    JxrCbpPredictorDecode(state);

    if (JxrHpDecoderDecodeCoefficients(state, macroblockX, macroblockY) != ICERR_OK)
        goto ErrorExit;

    if (codec->m_bResetContext) {
        JxrAdaptiveHuffmanAdapt(state->cbpHuffman);
        JxrAdaptiveHuffmanAdapt(state->cbpCountHuffman);
        for (tableIndex = 0; tableIndex < CONTEXTX; ++tableIndex) {
            JxrAdaptiveHuffmanAdapt(state->huffmanStates[tableIndex + CONTEXTX + CTDC]);
        }
    }

    return ICERR_OK;
ErrorExit:
    return ICERR_ERROR;
}

Int JxrHpDecoderDecodeMacroblock(CWMImageStrCodec* codec, CCodingContext* entropy,
    Int macroblockX, Int macroblockY)
{
    JxrDecoderSubbandContext state;
    JxrDecoderSubbandContextInit(&state, codec, entropy);
    return JxrHpDecoderDecodeSubband(&state, macroblockX, macroblockY);
}
