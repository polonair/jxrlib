#include "JxrLpDecoder.h"
#include "decode.h"
#include "JxrAdaptiveScan.h"
#include "JxrCoefficientBuffer.h"
#include "JxrEntropyBlockDecoder.h"
#include "JxrEntropyReader.h"

U8 decodeQPIndex(BitIOInfo* input, U8 bitCount);

Int JxrLpDecoderDecodeMacroblock(CWMImageStrCodec * pSC, CCodingContext *pContext,
        Int iMBX, Int iMBYdummy)
{
    const COLORFORMAT cf = pSC->m_param.cfColorFormat;
    const Int iChannels = (Int) pSC->m_param.cNumChannels;
    const Int iFullPlanes = (cf == YUV_420 || cf == YUV_422) ? 2 : iChannels;
    Int k;
	CAdaptiveScan *pScan = pContext->m_aScanLowpass;
    BitIOInfo* pIO = pContext->m_pIOLP;
    Int iModelBits = pContext->m_aModelLP.m_iFlcBits[0];
    Int aRLCoeffs[32], iNumNonzero = 0, iIndex = 0;
    Int aLaplacianMean[2] = { 0, 0}, *pLM = aLaplacianMean;
    Int iChannel, iCBP = 0;
#ifndef ARMOPT_BITIO    // ARM opt always uses 32-bit version of getBits
    U32 (*getBits)(BitIOInfo* pIO, U32 cBits) = JxrEntropyReaderRead;
#endif
    CWMIMBInfo * pMBInfo = &pSC->MBInfo;
    I32 *aDC[MAX_CHANNELS];

    UNREFERENCED_PARAMETER( iMBX );
    UNREFERENCED_PARAMETER( iMBYdummy );

    readIS_L1(pSC, pIO);
    if((pSC->WMISCP.bfBitstreamFormat != SPATIAL) && (pSC->pTile[pSC->cTileColumn].cBitsLP > 0))  // MB-based LP QP index
        pMBInfo->iQIndexLP = decodeQPIndex(pIO, pSC->pTile[pSC->cTileColumn].cBitsLP);

    // set arrays
    for (k = 0; k < (Int) pSC->m_param.cNumChannels; k++) {
        aDC[k & 15] = pMBInfo->iBlockDC[k];
    }
    /** reset adaptive scan totals **/
    if (pSC->m_bResetRGITotals) {
        JxrAdaptiveScanResetTotals(pScan, 16);
    }

    /** in raw mode, this can take 6% of the bits in the extreme low rate case!!! **/
    if (cf == YUV_420 || cf == YUV_422 || cf == YUV_444) {
        int iCountM = pContext->m_iCBPCountMax, iCountZ = pContext->m_iCBPCountZero;
        int iMax = iFullPlanes * 4 - 5; /* actually (1 << iNChannels) - 1 **/
        if (iCountZ <= 0 || iCountM < 0) {
            iCBP = 0;
            if (JxrEntropyReaderReadFlag(pIO)) {
                iCBP = 1;
                k = JxrEntropyReaderRead(pIO, iFullPlanes - 1);
                if (k) {
                    iCBP = k * 2 + JxrEntropyReaderRead(pIO, 1);
                }
            }
            if (iCountM < iCountZ)
                iCBP = iMax - iCBP;
        }
        else {
            iCBP = JxrEntropyReaderRead(pIO, iFullPlanes);
        }

        iCountM += 1 - 4 * (iCBP == iMax);//(b + c - 2*a);
        iCountZ += 1 - 4 * (iCBP == 0);//(a + b - 2*c);
        if (iCountM < -8)
            iCountM = -8;
        else if (iCountM > 7)
            iCountM = 7;
        pContext->m_iCBPCountMax = iCountM;

        if (iCountZ < -8)
            iCountZ = -8;
        else if (iCountZ > 7)
            iCountZ = 7;
        pContext->m_iCBPCountZero = iCountZ;
    }
    else { /** 1 or N channel **/
        for (iChannel = 0; iChannel < iChannels; iChannel++)
            iCBP |= (getBits (pIO, 1) << iChannel);
    }

#ifndef ARMOPT_BITIO    // ARM opt always uses 32-bit version of getBits
    if (pContext->m_aModelLP.m_iFlcBits[0] > 14 || pContext->m_aModelLP.m_iFlcBits[1] > 14) {
        getBits = getBit32;
    }
#endif

    for (iChannel = 0; iChannel < iFullPlanes; iChannel++) {
        JxrCoefficientBuffer coefficients = JxrCoefficientBufferCreate(aDC[iChannel], 0, 16);

        if (iCBP & 1) {
            iNumNonzero = JxrEntropyBlockDecoderDecodeLowpassBlock(iChannel > 0, aRLCoeffs, pContext->m_pAHexpt,
                CTDC, pIO, 1 + 9 * ((cf == YUV_420) && (iChannel == 1))
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
                    if (aDC[1][k] > 0) {
                        aDC[1][k] <<= iModelBits;
                        aDC[1][k] += getBits (pIO, iModelBits);
                    }
                    else if (aDC[1][k] < 0) {
                        aDC[1][k] <<= iModelBits;
                        aDC[1][k] -= getBits (pIO, iModelBits);
                    }
                    else {
                        aDC[1][k] = getBits (pIO, iModelBits);
                        if (aDC[1][k] && JxrEntropyReaderReadFlag(pIO))
                            aDC[1][k] = -aDC[1][k];
                    }

                    if (aDC[2][k] > 0) {
                        aDC[2][k] <<= iModelBits;
                        aDC[2][k] += getBits (pIO, iModelBits);
                    }
                    else if (aDC[2][k] < 0) {
                        aDC[2][k] <<= iModelBits;
                        aDC[2][k] -= getBits (pIO, iModelBits);
                    }
                    else {
                        aDC[2][k] = getBits (pIO, iModelBits);
                        if (aDC[2][k] && JxrEntropyReaderReadFlag(pIO))
                            aDC[2][k] = -aDC[2][k];
                    }
                }
            }
            else {
#ifdef WIN32
                const Int iMask = (1 << iModelBits) - 1;
#endif // WIN32
                for (k = 1; k < 16; k++) {
                    PixelI coefficient = JxrCoefficientBufferGet(&coefficients, k);
#ifdef WIN32
                    if (coefficient) {
                        Int r1 = _rotl(coefficient, iModelBits);
                        coefficient = (r1 ^ getBits(pIO, iModelBits)) - (r1 & iMask);
                        JxrCoefficientBufferSet(&coefficients, k, coefficient);
                    }
#else // WIN32
                    if (coefficient > 0) {
                        coefficient <<= iModelBits;
                        coefficient += getBits (pIO, iModelBits);
                        JxrCoefficientBufferSet(&coefficients, k, coefficient);
                    }
                    else if (coefficient < 0) {
                        coefficient <<= iModelBits;
                        coefficient -= getBits (pIO, iModelBits);
                        JxrCoefficientBufferSet(&coefficients, k, coefficient);
                    }
#endif // WIN32
                    else {
                        Int r1 = JxrEntropyReaderPeek(pIO, iModelBits + 1);
                        coefficient = ((r1 >> 1) ^ (-(r1 & 1))) + (r1 & 1);
                        JxrCoefficientBufferSet(&coefficients, k, coefficient);
                        JxrEntropyReaderConsume(pIO, iModelBits + (coefficient != 0));
                    }
                }
            }
        }
        pLM = aLaplacianMean + 1;
        iModelBits = pContext->m_aModelLP.m_iFlcBits[1];

        iCBP >>= 1;
    }

    UpdateModelMB (cf, iChannels, aLaplacianMean, &(pContext->m_aModelLP));

    if (pSC->m_bResetContext) {
        AdaptLowpassDec(pContext);
    }

    return ICERR_OK;
}
