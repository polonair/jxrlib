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
#include "JxrQuantizationIndexReader.h"
#include "JxrSubbandStreamRefill.h"

extern const int dctIndex[3][16];

/*************************************************************************
    DecodeCBP
*************************************************************************/
Bool JxrHpDecoderDecodeCbp(JxrDecoderSubbandContext* state)
{
    JxrDecoderFormatState* format = &state->formatState;
    JxrEntropyBitReader* reader = &state->highpassReader;
    const COLORFORMAT cf = JxrDecoderFormatStateGetColorFormat(format);
    const Int iChannel = (cf == NCOMPONENT || cf == CMYK) ? JxrDecoderFormatStateGetChannelCount(format) : 1;
    Int iCBPCY, iCBPCU , iCBPCV;
    Int k, iBlock, i;
    Int iNumCBP;
    Bool bIsChroma;
    CAdaptiveHuffman* pAHCBP = JxrHighpassCbpStateGetPatternHuffman(&state->highpassCbpState);
    CAdaptiveHuffman* pAHCBP1 = JxrHighpassCbpStateGetCountHuffman(&state->highpassCbpState);
    CAdaptiveHuffman* pAHex1 = JxrHuffmanStateSetGet(&state->huffmanStateSet, 1);

    if (!JxrDecoderFormatStateRefillLevel1(format, reader)) return FALSE;

    for (i = 0; i < iChannel; i++) {

        iCBPCY = iCBPCU = iCBPCV = 0;
        iNumCBP = JxrAdaptiveHuffmanDecodeShortTableReader(pAHCBP1->m_hufDecTable, reader);
        JxrAdaptiveHuffmanObserve(pAHCBP1, iNumCBP);

        switch (iNumCBP) {
            case 2:
                iNumCBP = JxrEntropyBitReaderRead(reader, 2);
                if (iNumCBP == 0)
                    iNumCBP = 3;
                else if (iNumCBP == 1)
                    iNumCBP = 5;
                else {
                    static const Int aTab[] = { 6, 9, 10, 12 };
                    iNumCBP = aTab[iNumCBP * 2 + JxrEntropyBitReaderReadFlag(reader) - 4];
                }
                break;
            case 1:
                iNumCBP = 1 << JxrEntropyBitReaderRead(reader, 2);
                break;
            case 3:
                iNumCBP = 0xf ^ (1 << JxrEntropyBitReaderRead(reader, 2));
                break;
            case 4:
                iNumCBP = 0xf;
        }

        for (iBlock = 0; iBlock < 4; iBlock++) {
            if (iNumCBP & (1 << iBlock)) {
                static const UInt gFLC0[] = { 0,2,1,2,2,0 };
                static const UInt gOff0[] = { 0,4,2,8,12,1 };
                static const UInt gOut0[] = { 0,15,3,12, 1,2,4,8, 5,6,9,10, 7,11,13,14 };
                Int iNumBlockCBP = JxrAdaptiveHuffmanDecodeReader(pAHCBP, reader);
                unsigned int val = (unsigned int) iNumBlockCBP + 1, iCode1;

                /* Observation is performed by JxrAdaptiveHuffmanDecode. */
                iNumBlockCBP = 0;

                if (val >= 6) { // chroma present
                    if (JxrEntropyBitReaderReadFlag(reader)) {
                        iNumBlockCBP = 0x10;
                    }
                    else if (JxrEntropyBitReaderReadFlag(reader)) {
                        iNumBlockCBP = 0x20;
                    }
                    else {
                        iNumBlockCBP = 0x30;
                    }
                    if (val == 9) {
                        if (JxrEntropyBitReaderReadFlag(reader)) {
                            // do nothing
                        }
                        else if (JxrEntropyBitReaderReadFlag(reader)) {
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
                    iCode1 += JxrEntropyBitReaderRead(reader, gFLC0[val]);
                }
                iNumBlockCBP += gOut0[iCode1];

                switch (cf) {
                    case YUV_444:
                        iCBPCY |= ((iNumBlockCBP & 0xf) << (iBlock * 4));
                        for (k = 0; k < 2; k++) {
                            bIsChroma = ((iNumBlockCBP>>(k+4)) & 0x01);
                            if (bIsChroma) { // U is present in block
                                Int iCode = JxrAdaptiveHuffmanDecodeShortTableReader(pAHex1->m_hufDecTable, reader);
                                switch (iCode) {
                            case 1:
                                iCode = JxrEntropyBitReaderRead(reader, 2);
                                if (iCode == 0)
                                    iCode = 3;
                                else if (iCode == 1)
                                    iCode = 5;
                                else {
                                    static const Int aTab[] = { 6, 9, 10, 12 };
                                    iCode = aTab[iCode * 2 + JxrEntropyBitReaderReadFlag(reader) - 4];
                                }
                                break;
                            case 0:
                                iCode = 1 << JxrEntropyBitReaderRead(reader, 2);
                                break;
                            case 2:
                                iCode = 0xf ^ (1 << JxrEntropyBitReaderRead(reader, 2));
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
                                if(JxrEntropyBitReaderReadFlag(reader)) {
                                    iCode = 1;
                                }
                                else if(JxrEntropyBitReaderReadFlag(reader)){
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

        JxrMacroblockCbpStateSetDifferential(&state->macroblockCbpState, i, iCBPCY);
        if (cf == YUV_420 || cf == YUV_444 || cf == YUV_422) {
            JxrMacroblockCbpStateSetDifferential(&state->macroblockCbpState, 1, iCBPCU);
            JxrMacroblockCbpStateSetDifferential(&state->macroblockCbpState, 2, iCBPCV);
        }
    }
    return TRUE;
}

/*************************************************************************
    GetCoeffs
*************************************************************************/
static Int JxrHpDecoderDecodeCoefficients(JxrDecoderSubbandContext* state,
    Int macroblockX, Int macroblockY)
{
    JxrDecoderFormatState* format = &state->formatState;
    JxrMacroblockState* macroblock = &state->macroblockState;
    const JxrDecoderTileState* tile = JxrDecoderFormatStateGetCurrentTile(format);
    JxrEntropyBitReader* highpassReader = &state->highpassReader;
    JxrEntropyBitReader* flexbitsReader = &state->flexbitsReader;
    const COLORFORMAT cf = JxrDecoderFormatStateGetColorFormat(format);
    const Int iChannels = JxrDecoderFormatStateGetChannelCount(format);
    const Int iPlanes = (cf == YUV_420 || cf == YUV_422) ? 1 : iChannels;
    Int  iQP;
    JxrAdaptiveScanState* scanState;
    JxrCoefficientBuffer coefficients;
    Int i, iBlock, iSubblock, iNBlocks = 4;
    Int iModelBits = JxrAdaptiveModelStateGetFlcBits(&state->highpassModelState, 0);
    Int aLaplacianMean[2] = { 0, 0}, *pLM = aLaplacianMean + 0;
    const Int *pOrder = dctIndex[0];
    const Int iOrient = JxrMacroblockStateGetOrientation(macroblock);
    Bool bChroma = FALSE;

    Int iCBPCU = JxrMacroblockCbpStateGetCbp(&state->macroblockCbpState, 1);
    Int iCBPCV = JxrMacroblockCbpStateGetCbp(&state->macroblockCbpState, 2);
    Int iCBPCY = JxrMacroblockCbpStateGetCbp(&state->macroblockCbpState, 0);

    UNREFERENCED_PARAMETER(macroblockX);
    UNREFERENCED_PARAMETER(macroblockY);

    /** set scan arrays and other MB level constants **/
    if (iOrient == 1) {
        scanState = &state->verticalScanState;
    }
    else {
        scanState = &state->horizontalScanState;
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

        if (JxrDecoderFormatStateHasFlexbits(format) &&
            !JxrDecoderFormatStateRefillLevel1(format, flexbitsReader))
            return ICERR_ERROR;

        for (iBlock = 0; iBlock < iNBlocks; iBlock++) {

            if (!JxrDecoderFormatStateRefillLevel2(format, highpassReader)) return ICERR_ERROR;
            if (!JxrEntropyBitReaderSharesStream(highpassReader, flexbitsReader))
                if (!JxrDecoderFormatStateRefillLevel2(format, flexbitsReader)) return ICERR_ERROR;

            iQP = JxrDecoderFormatStateIsTranscode(format) ? 1 :
                JxrDecoderTileStateGetHighpassQuantizerParameter(tile,
                    iPlanes > 1 ? i : (iBlock > 3 ?
                    (cf == YUV_420 ? iBlock - 3 : iBlock / 2 - 1) : 0),
                    JxrMacroblockStateGetHighpassQuantizerIndex(macroblock));

            for (iSubblock = 0; iSubblock < 4; iSubblock++, iIndex++, iCBPCY >>= 1) {
                coefficients = JxrHpCoefficientBlockResolverResolve(state, i, iBlock,
                    iSubblock, iIndex);

                /** read AC values **/
                assert (!JxrDecoderFormatStateShouldSkipFlexbits(format) || !JxrDecoderFormatStateIsSpatial(format) || !JxrDecoderFormatStateHasFlexbits(format));
                {
                    JxrHpBlockDecodingContext blockContext;
                    blockContext.huffmanStateSet = &state->huffmanStateSet;
                    blockContext.highpassReader = highpassReader;
                    blockContext.flexbitsReader = flexbitsReader;
                    blockContext.coefficientBuffer = &coefficients;
                    blockContext.scanState = scanState;
                    blockContext.coefficientOrder = pOrder;
                    blockContext.isChroma = bChroma;
                    blockContext.hasCoefficients = (iCBPCY & 1) != 0;
                    blockContext.skipFlexbits = JxrDecoderFormatStateShouldSkipFlexbits(format);
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
                iModelBits = JxrAdaptiveModelStateGetFlcBits(&state->highpassModelState, 1);
                pLM = aLaplacianMean + 1;
                bChroma = TRUE;
            }
        }

        iCBPCY = JxrMacroblockCbpStateGetCbp(&state->macroblockCbpState,
            (i + 1) & 0xf);
        assert (MAX_CHANNELS == 16);
    }

    /** update model at end of MB **/
    JxrAdaptiveModelStateUpdateForMacroblock(&state->highpassModelState, cf,
        iChannels, aLaplacianMean);
    return ICERR_OK;
}


Int JxrHpDecoderDecodeSubband(JxrDecoderSubbandContext* state,
    Int macroblockX, Int macroblockY)
{
    JxrMacroblockState* macroblock = &state->macroblockState;
    JxrDecoderFormatState* format = &state->formatState;
    const JxrDecoderTileState* tile = JxrDecoderFormatStateGetCurrentTile(format);
    JxrEntropyBitReader* highpassReader = &state->highpassReader;
    Int tableIndex;

    /** reset adaptive scan totals **/
    if (JxrDecoderFormatStateShouldResetScan(format)) {
        JxrAdaptiveScanStateResetTotals(&state->horizontalScanState, 16);
        JxrAdaptiveScanStateResetTotals(&state->verticalScanState, 16);
    }
    if (!JxrDecoderFormatStateIsSpatial(format) &&
        JxrDecoderTileStateGetHighpassQuantizerBits(tile) > 0) { // MB-based HP QP index
        JxrMacroblockStateSetHighpassQuantizerIndex(macroblock,
            JxrQuantizationIndexReaderDecode(highpassReader,
                JxrDecoderTileStateGetHighpassQuantizerBits(tile)));
        if (JxrMacroblockStateGetHighpassQuantizerIndex(macroblock) >=
            JxrDecoderTileStateGetHighpassQuantizerCount(tile))
            goto ErrorExit;
    }
    else if(JxrDecoderTileStateGetHighpassQuantizerBits(tile) == 0 &&
        JxrDecoderTileStateGetHighpassQuantizerCount(tile) > 1) // use LP QP
        JxrMacroblockStateSetHighpassQuantizerIndex(macroblock,
            JxrMacroblockStateGetLowpassQuantizerIndex(macroblock));


    if (!JxrHpDecoderDecodeCbp(state)) goto ErrorExit;
    JxrCbpPredictorDecode(state);

    if (JxrHpDecoderDecodeCoefficients(state, macroblockX, macroblockY) != ICERR_OK)
        goto ErrorExit;

    if (JxrDecoderFormatStateShouldResetContext(format)) {
        JxrHighpassCbpStateAdapt(&state->highpassCbpState);
        for (tableIndex = 0; tableIndex < CONTEXTX; ++tableIndex) {
            JxrHuffmanStateSetAdapt(&state->huffmanStateSet,
                tableIndex + CONTEXTX + CTDC);
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
