#include "JxrLpDecoder.h"
#include "decode.h"
#include "JxrAdaptiveScan.h"
#include "JxrCoefficientBuffer.h"
#include "JxrEntropyBlockDecoder.h"
#include "JxrEntropyReader.h"
#include "JxrLpResidualDecoder.h"
#include "JxrAdaptiveHuffman.h"
#include "JxrQuantizationIndexReader.h"
#include "JxrSubbandStreamRefill.h"

Int JxrLpDecoderDecodeSubband(JxrDecoderSubbandContext* state,
        Int macroblockX, Int macroblockY)
{
    CWMImageStrCodec* codec = state->codec;
    const COLORFORMAT cf = codec->m_param.cfColorFormat;
    const Int iChannels = (Int) codec->m_param.cNumChannels;
    const Int iFullPlanes = (cf == YUV_420 || cf == YUV_422) ? 2 : iChannels;
    Int k;
    CAdaptiveScan* pScan = state->lowpassScan;
    JxrEntropyBitReader* reader = &state->lowpassReader;
    Int iModelBits = JxrAdaptiveModelStateGetFlcBits(&state->lowpassModelState, 0);
    Int aRLCoeffs[32], iNumNonzero = 0, iIndex = 0;
    Int aLaplacianMean[2] = { 0, 0}, *pLM = aLaplacianMean;
    Int iChannel, iCBP = 0;
    CWMIMBInfo* pMBInfo = &codec->MBInfo;
    I32 *aDC[MAX_CHANNELS];

    UNREFERENCED_PARAMETER(macroblockX);
    UNREFERENCED_PARAMETER(macroblockY);

    JxrSubbandStreamRefillLevel1(codec, reader);
    if((codec->WMISCP.bfBitstreamFormat != SPATIAL) && (codec->pTile[codec->cTileColumn].cBitsLP > 0))  // MB-based LP QP index
        pMBInfo->iQIndexLP = JxrQuantizationIndexReaderDecode(reader,
            codec->pTile[codec->cTileColumn].cBitsLP);

    // set arrays
    for (k = 0; k < (Int) codec->m_param.cNumChannels; k++) {
        aDC[k & 15] = pMBInfo->iBlockDC[k];
    }
    /** reset adaptive scan totals **/
    if (codec->m_bResetRGITotals) {
        JxrAdaptiveScanResetTotals(pScan, 16);
    }

    /** in raw mode, this can take 6% of the bits in the extreme low rate case!!! **/
    if (cf == YUV_420 || cf == YUV_422 || cf == YUV_444) {
        Int iCountM = JxrLowpassCbpStateGetMaxCount(&state->lowpassCbpState);
        Int iCountZ = JxrLowpassCbpStateGetZeroCount(&state->lowpassCbpState);
        Int iMax = iFullPlanes * 4 - 5; /* actually (1 << iNChannels) - 1 **/
        if (iCountZ <= 0 || iCountM < 0) {
            iCBP = 0;
            if (JxrEntropyBitReaderReadFlag(reader)) {
                iCBP = 1;
                k = JxrEntropyBitReaderRead(reader, iFullPlanes - 1);
                if (k) {
                    iCBP = k * 2 + JxrEntropyBitReaderRead(reader, 1);
                }
            }
            if (iCountM < iCountZ)
                iCBP = iMax - iCBP;
        }
        else {
            iCBP = JxrEntropyBitReaderRead(reader, iFullPlanes);
        }

        JxrLowpassCbpStateObserve(&state->lowpassCbpState, iCBP, iMax);
    }
    else { /** 1 or N channel **/
        for (iChannel = 0; iChannel < iChannels; iChannel++)
            iCBP |= ((Int)JxrLpResidualDecoderReadBitsReader(reader, 1) << iChannel);
    }

    for (iChannel = 0; iChannel < iFullPlanes; iChannel++) {
        JxrCoefficientBuffer coefficients = JxrCoefficientBufferCreate(aDC[iChannel], 0, 16);

        if (iCBP & 1) {
            iNumNonzero = JxrEntropyBlockDecoderDecodeLowpassBlockReader(iChannel > 0, aRLCoeffs, state->huffmanStates,
                CTDC, reader, 1 + 9 * ((cf == YUV_420) && (iChannel == 1))
                + ((cf == YUV_422) && (iChannel == 1)));

            if ((cf == YUV_420 || cf == YUV_422) && iChannel) {
                Int aTemp[16]; //14 required, 16 for security
                static const Int aRemap[] = { 4,  1,2,3,  5,6,7 };
                const Int *pRemap = aRemap + (cf == YUV_420);
                const Int iCount = (cf == YUV_420) ? 6 : 14;

                (*pLM) += iNumNonzero;
                iIndex = 0;
                memset (aTemp, 0, sizeof(aTemp));

                for (k = 0; k < iNumNonzero; k++) {
                    iIndex += aRLCoeffs[k * 2];
                    aTemp[iIndex & 0xf] = aRLCoeffs[k * 2 + 1];
                    iIndex++;
                }

                for (k = 0; k < iCount; k++) {
                    aDC[(k & 1) + 1][pRemap[k >> 1]] = aTemp[k];
                }
            }
            else {
                (*pLM) += iNumNonzero;
                iIndex = 1;

                for (k = 0; k < iNumNonzero; k++) {
                    iIndex += aRLCoeffs[k * 2];
                    JxrCoefficientBufferSet(&coefficients, JxrAdaptiveScanGetCoefficientIndex(pScan, iIndex), aRLCoeffs[k * 2 + 1]);
                    JxrAdaptiveScanObserveNonZero(pScan, iIndex);
                    iIndex++;
                }
            }
        }

        if (iModelBits) {
            if ((cf == YUV_420 || cf == YUV_422) && iChannel) {
                for (k = 1; k < (cf == YUV_420 ? 4 : 8); k++) {
                    aDC[1][k] = JxrLpResidualDecoderDecodeChromaCoefficientReader(aDC[1][k], reader, iModelBits);
                    aDC[2][k] = JxrLpResidualDecoderDecodeChromaCoefficientReader(aDC[2][k], reader, iModelBits);
                }
            }
            else {
                for (k = 1; k < 16; k++) {
                    PixelI coefficient = JxrCoefficientBufferGet(&coefficients, k);
                    JxrCoefficientBufferSet(&coefficients, k,
                        JxrLpResidualDecoderDecodeNormalCoefficientReader(coefficient, reader, iModelBits));
                }
            }
        }
        pLM = aLaplacianMean + 1;
        iModelBits = JxrAdaptiveModelStateGetFlcBits(&state->lowpassModelState, 1);

        iCBP >>= 1;
    }

    JxrAdaptiveModelStateUpdateForMacroblock(&state->lowpassModelState, cf,
        iChannels, aLaplacianMean);

    if (codec->m_bResetContext) {
        for (k = 0; k < CONTEXTX + CTDC; ++k) {
            JxrAdaptiveHuffmanAdapt(state->huffmanStates[k]);
        }
    }

    return ICERR_OK;
}

Int JxrLpDecoderDecodeMacroblock(CWMImageStrCodec* codec, CCodingContext* entropy,
    Int macroblockX, Int macroblockY)
{
    JxrDecoderSubbandContext state;
    JxrDecoderSubbandContextInit(&state, codec, entropy);
    return JxrLpDecoderDecodeSubband(&state, macroblockX, macroblockY);
}
