//*@@@+++@@@@******************************************************************
//
// Copyright © Microsoft Corp.
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 
// • Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
// • Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
//*@@@---@@@@******************************************************************

#include "strcodec.h"
#include "decode.h"
#include "JxrEntropyReader.h"
#include "JxrAdaptiveHuffman.h"
#include "JxrAdaptiveScan.h"
#include "JxrCoefficientBuffer.h"
#include "JxrEntropyLevelDecoder.h"
#include "JxrDcDecoder.h"
#include "JxrEntropyBlockDecoder.h"
#include "JxrLpDecoder.h"

#ifdef MEM_TRACE
#define TRACE_MALLOC    1
#define TRACE_NEW       0
#define TRACE_HEAP      0
#include "memtrace.h"
#endif

extern const int dctIndex[3][16];
extern const int blkOffset[16];
extern const int blkOffsetUV[4];

//#undef X86OPT_INLINE

#ifdef X86OPT_INLINE
#define _FORCEINLINE __forceinline
#else // X86OPT_INLINE
#define _FORCEINLINE
#endif // X86OPT_INLINE

//================================================================
// Decoder bit reader compatibility helpers
//================================================================
static U32 _peekBit16(BitIOInfo* pIO, U32 cBits)
{
    return JxrEntropyReaderPeek(pIO, cBits);
}

static Void _flushBit16(BitIOInfo* pIO, U32 cBits)
{
    JxrEntropyReaderConsume(pIO, cBits);
}

static U32 _getBit16(BitIOInfo* pIO, U32 cBits)
{
    return JxrEntropyReaderRead(pIO, cBits);
}

#define SIGN_BIT(TypeOrValue) (((UInt) 1) << (8 * sizeof (TypeOrValue) - 1))
/***********************************************************************************************************
  Huffman decode (input is a fully built Huffman table)
***********************************************************************************************************/
Int getHuff(const short *pDecodeTable, BitIOInfo* pIO)
{
    Int iSymbol, iSymbolHuff;
    iSymbol = pDecodeTable[peekBit16(pIO, HUFFMAN_DECODE_ROOT_BITS)];

    flushBit16(pIO, iSymbol < 0 ? HUFFMAN_DECODE_ROOT_BITS : iSymbol & ((1 << HUFFMAN_DECODE_ROOT_BITS_LOG) - 1));
	iSymbolHuff = iSymbol >> HUFFMAN_DECODE_ROOT_BITS_LOG;

	if (iSymbolHuff < 0) {
		iSymbolHuff = iSymbol;
        while ((iSymbolHuff = pDecodeTable[iSymbolHuff + SIGN_BIT (pDecodeTable[0]) + getBit16(pIO, 1)]) < 0);
	}
    return (iSymbolHuff);
}

static U32 _getBool16(BitIOInfo* pIO)
{
    return JxrEntropyReaderReadFlag(pIO);
}

static I32 _getSign(BitIOInfo* pIO)
{
    return JxrEntropyReaderReadSign(pIO);
}

/** this function returns cBits if zero is read, or a signed value if first cBits are not all zero **/
static I32 _getBit16s(BitIOInfo* pIO, U32 cBits)
{
    return JxrEntropyReaderReadSignedResidual(pIO, cBits);
}

/*************************************************************************
    Huffman decoding with short tables
*************************************************************************/
static _FORCEINLINE Int _getHuffShort(const short *pDecodeTable, BitIOInfo* pIO)
{
    Int iSymbol = pDecodeTable[_peekBit16(pIO, HUFFMAN_DECODE_ROOT_BITS)];
    assert(iSymbol >= 0);
    // for some strange reason, inlining flushBit doesn't work well
    flushBit16(pIO, iSymbol & ((1 << HUFFMAN_DECODE_ROOT_BITS_LOG) - 1));
    return (iSymbol >> HUFFMAN_DECODE_ROOT_BITS_LOG);
}
/*************************************************************************
    Adapt + Huffman init
*************************************************************************/
static Int AdaptDecFixed (CAdaptiveHuffman *pAH)
{
    JxrAdaptiveHuffmanAdapt(pAH);
    return ICERR_OK;
}

/*************************************************************************
    DecodeCBP
*************************************************************************/
static Void DecodeCBP(CWMImageStrCodec * pSC, CCodingContext *pContext)
{
    BitIOInfo* pIO = pContext->m_pIOAC;
    const COLORFORMAT cf = pSC->m_param.cfColorFormat;
    const Int iChannel = (cf == NCOMPONENT || cf == CMYK) ? (Int) pSC->m_param.cNumChannels : 1;
    Int iCBPCY, iCBPCU , iCBPCV;
    Int k, iBlock, i;
    Int iNumCBP;
    Bool bIsChroma;
    CAdaptiveHuffman *pAHCBP = pContext->m_pAdaptHuffCBPCY;
    CAdaptiveHuffman *pAHCBP1 = pContext->m_pAdaptHuffCBPCY1;
    CAdaptiveHuffman *pAHex1 = pContext->m_pAHexpt[1];
        
    readIS_L1(pSC, pIO);

    for (i = 0; i < iChannel; i++) {

        iCBPCY = iCBPCU = iCBPCV = 0;
        iNumCBP = JxrAdaptiveHuffmanDecodeShortTable(pAHCBP1->m_hufDecTable, pIO);
        JxrAdaptiveHuffmanObserve(pAHCBP1, iNumCBP);

        switch (iNumCBP) {
            case 2:
                iNumCBP = _getBit16(pIO, 2);
                if (iNumCBP == 0)
                    iNumCBP = 3;
                else if (iNumCBP == 1)
                    iNumCBP = 5;
                else {
                    static const Int aTab[] = { 6, 9, 10, 12 };
                    iNumCBP = aTab[iNumCBP * 2 + _getBool16 (pIO) - 4];
                }
                break;
            case 1:
                iNumCBP = 1 << _getBit16(pIO, 2);
                break;
            case 3:
                iNumCBP = 0xf ^ (1 << _getBit16(pIO, 2));
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
                    if (_getBool16 (pIO)) {
                        iNumBlockCBP = 0x10;
                    }
                    else if (_getBool16 (pIO)) {
                        iNumBlockCBP = 0x20;
                    }
                    else {
                        iNumBlockCBP = 0x30;
                    }
                    if (val == 9) {
                        if (_getBool16 (pIO)) {
                            // do nothing
                        }
                        else if (_getBool16 (pIO)) {
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
                    iCode1 += _getBit16(pIO, gFLC0[val]);
                }
                iNumBlockCBP += gOut0[iCode1];

                switch (cf) {
                    case YUV_444:
                        iCBPCY |= ((iNumBlockCBP & 0xf) << (iBlock * 4));
                        for (k = 0; k < 2; k++) {
                            bIsChroma = ((iNumBlockCBP>>(k+4)) & 0x01);
                            if (bIsChroma) { // U is present in block
                                Int iCode = _getHuffShort(pAHex1->m_hufDecTable, pIO);
                                switch (iCode) { 
                            case 1:
                                iCode = _getBit16(pIO, 2);
                                if (iCode == 0)
                                    iCode = 3;
                                else if (iCode == 1)
                                    iCode = 5;
                                else {
                                    static const Int aTab[] = { 6, 9, 10, 12 };
                                    iCode = aTab[iCode * 2 + _getBool16 (pIO) - 4];
                                }
                                break;
                            case 0:
                                iCode = 1 << _getBit16(pIO, 2);
                                break;
                            case 2:
                                iCode = 0xf ^ (1 << _getBit16(pIO, 2));
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
                                if(_getBool16(pIO)) {
                                    iCode = 1;
                                }
                                else if(_getBool16(pIO)){
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

        pSC->MBInfo.iDiffCBP[i] = iCBPCY;
        if (cf == YUV_420 || cf == YUV_444 || cf == YUV_422) {
            pSC->MBInfo.iDiffCBP[1] = iCBPCU;
            pSC->MBInfo.iDiffCBP[2] = iCBPCV;
        }
    }
}

/*************************************************************************
    DecodeBlockHighpass : 
*************************************************************************/
static _FORCEINLINE Int DecodeBlockHighpass (const Bool bChroma, struct CAdaptiveHuffman **pAHexpt,
                       BitIOInfo* pIO, const Int iQP, JxrCoefficientBuffer *coefficients, CAdaptiveScan *pScan)
{
    const Int iContextOffset = CTDC + CONTEXTX;
    UInt  iLoc = 1;
    Int iSR, iSRn, iIndex, iNumNonzero = 1, iCont, iSign, iLevel;
    struct CAdaptiveHuffman **pAH1 = pAHexpt + iContextOffset + bChroma * 3;
    const CAdaptiveScan *pConstScan = (const CAdaptiveScan *) pScan;

    /** first symbol **/
    iIndex = JxrEntropyBlockDecoderDecodeFirstSymbol(pAH1[0], pIO);
    iSR = (iIndex & 1);
    iSRn = iIndex >> 2;

    iCont = iSR & iSRn;
    iSign = _getSign(pIO);

    iLevel = (iQP ^ iSign) - iSign;
    if (iIndex & 2 /* iSL */) {
        iLevel *= JxrEntropyLevelDecoderDecode (pAHexpt[6 + iContextOffset + iCont], pIO);// ^ iSign) - iSign;
    }
    //else {
    //    iLevel = (1 | iSign); // 0 -> 1; -1 -> -1
    //}
    if (iSR == 0) {
       iLoc += JxrEntropyBlockDecoderDecodeRun(15 - iLoc, pAHexpt[0], pIO);
    }
    iLoc &= 0xf;
    JxrCoefficientBufferSet(coefficients, JxrAdaptiveScanGetCoefficientIndex(pConstScan, iLoc), (PixelI)iLevel);//(PixelI)(iQP * iLevel);
    JxrAdaptiveScanObserveNonZero(pScan, iLoc);
    iLoc = (iLoc + 1) & 0xf;
    //iLoc++;

    while (iSRn != 0) {
        iSR = iSRn & 1;
        if (iSR == 0) {
            iLoc += JxrEntropyBlockDecoderDecodeRun(15 - iLoc, pAHexpt[0], pIO);
            if (iLoc >= 16)
                return 16;
        }
        iIndex = JxrEntropyBlockDecoderDecodeNextSymbol(iLoc + 1, pAH1[iCont + 1], pIO);
        iSRn = iIndex >> 1;

        assert (iSRn >= 0 && iSRn < 3);
        iCont &= iSRn;  /** huge difference! **/
        iSign = _getSign(pIO);

        iLevel = (iQP ^ iSign) - iSign;
        if (iIndex & 1 /* iSL */) {
            iLevel *= JxrEntropyLevelDecoderDecode (pAHexpt[6 + iContextOffset + iCont], pIO);// ^ iSign) - iSign;
            //iLevel = (JxrEntropyLevelDecoderDecode (pAHexpt[6 + iContextOffset + iCont], pIO) ^ iSign) - iSign;
        }
        //else {
        //    iLevel = (1 | iSign); // 0 -> 1; -1 -> -1 (was 1 + (iSign * 2))
        //}
    JxrCoefficientBufferSet(coefficients, JxrAdaptiveScanGetCoefficientIndex(pConstScan, iLoc), (PixelI)iLevel);//(PixelI)(iQP * iLevel);
    JxrAdaptiveScanObserveNonZero(pScan, iLoc);

        iLoc = (iLoc + 1) & 0xf;
        iNumNonzero++;
    }
    return iNumNonzero;
}

/*************************************************************************
    DecodeBlockAdaptive
*************************************************************************/
static _FORCEINLINE Int DecodeBlockAdaptive (Bool bNoSkip, Bool bChroma, CAdaptiveHuffman **pAdHuff,
                                BitIOInfo *pIO, BitIOInfo *pIOFL,
                                JxrCoefficientBuffer *coefficients, CAdaptiveScan *pScan,
                                const Int iModelBits, const Int iTrim, const Int iQP,
                                const Int *pOrder, const Bool bSkipFlexbits)
{
    // const Int iLocation = 1;
    // const Int iContextOffset = CTDC + CONTEXTX;
    Int kk, iNumNonzero = 0, iFlex = iModelBits - iTrim;

    if (iFlex < 0 || bSkipFlexbits)
        iFlex = 0;

    if (bNoSkip) {
        const Int iQP1 = (iQP << iModelBits);
        iNumNonzero = DecodeBlockHighpass (bChroma, pAdHuff, pIO, iQP1, coefficients, pScan);
    }
    if (iFlex) {
        UInt k;
        if (iQP + iTrim == 1) { // only iTrim = 0, iQP = 1 is legal
            assert (iTrim == 0);
            assert (iQP == 1);

            for (k = 1; k < 16; k++) {
                PixelI coefficient = JxrCoefficientBufferGet(coefficients, pOrder[k]);
                if (coefficient < 0) {
                    Int fine = _getBit16(pIOFL, iFlex);
                    JxrCoefficientBufferAdd(coefficients, pOrder[k], (PixelI)(-fine));
                }
                else if (coefficient > 0) {
                    Int fine = _getBit16(pIOFL, iFlex);
                    JxrCoefficientBufferAdd(coefficients, pOrder[k], (PixelI)fine);
                }
                else {
                    JxrCoefficientBufferSet(coefficients, pOrder[k], (PixelI)_getBit16s(pIOFL, iFlex));
                }
            }
        }
        else {
            const Int iQP1 = iQP << iTrim;
            for (k = 1; k < 16; k++) {
                kk = JxrCoefficientBufferGet(coefficients, pOrder[k]);
                if (kk < 0) {
                    Int fine = _getBit16(pIOFL, iFlex);
                    JxrCoefficientBufferAdd(coefficients, pOrder[k], (PixelI)(-iQP1 * fine));
                }
                else if (kk > 0) {
                    Int fine = _getBit16(pIOFL, iFlex);
                    JxrCoefficientBufferAdd(coefficients, pOrder[k], (PixelI)(iQP1 * fine));
                }
                else {
                    JxrCoefficientBufferSet(coefficients, pOrder[k], (PixelI)(iQP1 * _getBit16s(pIOFL, iFlex)));
                }
            }
        }
    }

    return iNumNonzero;
}


/*************************************************************************
    GetCoeffs
*************************************************************************/
static _FORCEINLINE Int DecodeCoeffs (CWMImageStrCodec * pSC, CCodingContext *pContext,
                         Int iMBX, Int iMBY,
                         BitIOInfo* pIO, BitIOInfo *pIOFL)
{
    CWMITile * pTile = pSC->pTile + pSC->cTileColumn;
    const COLORFORMAT cf = pSC->m_param.cfColorFormat;
    const Int iChannels = (Int) pSC->m_param.cNumChannels;
    const Int iPlanes = (cf == YUV_420 || cf == YUV_422) ? 1 : iChannels;
    Int  iQP;
    CAdaptiveScan *pScan;
    PixelI  *pCoeffs;
    JxrCoefficientBuffer coefficients;
    Int i, iBlock, iSubblock, iNBlocks = 4;
    Int iModelBits = pContext->m_aModelAC.m_iFlcBits[0];
    Int aLaplacianMean[2] = { 0, 0}, *pLM = aLaplacianMean + 0;
    const Int *pOrder = dctIndex[0];
    const Int iOrient = pSC->MBInfo.iOrientation;
    Bool bChroma = FALSE;

    Int iCBPCU = pSC->MBInfo.iCBP[1];
    Int iCBPCV = pSC->MBInfo.iCBP[2];
    Int iCBPCY = pSC->MBInfo.iCBP[0];

    UNREFERENCED_PARAMETER( iMBX );
    UNREFERENCED_PARAMETER( iMBY );

    /** set scan arrays and other MB level constants **/
    if (iOrient == 1) {
        pScan = pContext->m_aScanVert;
    }
    else {
        pScan = pContext->m_aScanHoriz;
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

        if(pSC->WMISCP.sbSubband != SB_NO_FLEXBITS)
            readIS_L1(pSC, pIOFL);

        for (iBlock = 0; iBlock < iNBlocks; iBlock++) {

            readIS_L2(pSC, pIO);
            if (pIO != pIOFL)
                readIS_L2(pSC, pIOFL);

            iQP = (pSC->m_param.bTranscode ? 1 : pTile->pQuantizerHP[iPlanes > 1 ? i : (iBlock > 3 ? (cf == YUV_420 ? iBlock - 3 : iBlock / 2 - 1) : 0)][pSC->MBInfo.iQIndexHP].iQP);

            for (iSubblock = 0; iSubblock < 4; iSubblock++, iIndex++, iCBPCY >>= 1) {
                pCoeffs = pSC->p1MBbuffer[i] + blkOffset[iIndex & 0xf];

                //if (iBlock < 4) {//(cf == YUV_444) {
                    //bBlockNoSkip = ((iTempCBPC & (1 << iIndex1)) != 0);
                    //pCoeffs = pSC->p1MBbuffer[iBlock >> 2] + blkOffset[iIndex & 0xf];
                //}
                //else {
                if (iBlock >= 4) {
                    if(cf == YUV_420) {
                        pCoeffs = pSC->p1MBbuffer[iBlock - 3] + blkOffsetUV[iSubblock];
                    }
                    else { // YUV_422
                        pCoeffs = pSC->p1MBbuffer[1 + (1 & (iBlock >> 1))] + ((iBlock & 1) * 32) + blkOffsetUV_422[iSubblock];
                    }
                }

                coefficients = JxrCoefficientBufferCreate(pCoeffs, 0, 16);

                /** read AC values **/
                assert (pSC->m_Dparam->bSkipFlexbits == 0 || pSC->WMISCP.bfBitstreamFormat == FREQUENCY || pSC->WMISCP.sbSubband == SB_NO_FLEXBITS);
                iNumNonZero = DecodeBlockAdaptive ((iCBPCY & 1), bChroma, pContext->m_pAHexpt,
                    pIO, pIOFL, &coefficients, pScan, iModelBits, pContext->m_iTrimFlexBits,
                    iQP, pOrder, pSC->m_Dparam->bSkipFlexbits);
                if(iNumNonZero > 16) // something is wrong!
                    return ICERR_ERROR;
                // shouldn't this be > 15?
                (*pLM) += iNumNonZero;
            }
            if (iBlock == 3) {
                iModelBits = pContext->m_aModelAC.m_iFlcBits[1];
                pLM = aLaplacianMean + 1;
                bChroma = TRUE;
            }
        }

        iCBPCY = pSC->MBInfo.iCBP[(i + 1) & 0xf];
        assert (MAX_CHANNELS == 16);
    }

    /** update model at end of MB **/
    UpdateModelMB (cf, iChannels, aLaplacianMean, &(pContext->m_aModelAC));
    return ICERR_OK;
}

U8 decodeQPIndex(BitIOInfo* pIO,U8 cBits)
{
    if(_getBit16(pIO, 1) == 0)
        return 0;
    return (U8)(_getBit16(pIO, cBits) + 1);
}

/*************************************************************************
    DecodeSecondStageCoeff
*************************************************************************/
Int DecodeMacroblockLowpass(CWMImageStrCodec* pSC, CCodingContext* pContext, Int iMBX, Int iMBYdummy)
{
    return JxrLpDecoderDecodeMacroblock(pSC, pContext, iMBX, iMBYdummy);
}

/*************************************************************************
    8 bit YUV 420 macroblock decode function with 4x4 transform
    Index order is as follows:
    Y:              U:      V:
     0  1  4  5     16 17   20 21
     2  3  6  7     18 19   22 23
     8  9 12 13
    10 11 14 15

    DCAC coefficients stored for 4x4 - offsets (x == no storage)
    Y:
    x x x [0..3]
    x x x [4..7]
    x x x [8..11]
    [16..19] [20..23] [24..27] [28..31,12..15]

    U, V:
    x [0..3]
    [8..11] [4..7,12..15]
*************************************************************************/
Int DecodeMacroblockDC(CWMImageStrCodec* pSC, CCodingContext* pContext, Int iMBX, Int iMBY)
{
    return JxrDcDecoderDecodeMacroblock(pSC, pContext, iMBX, iMBY);
}

/*************************************************************************
    DecodeMacroblockHighpass
*************************************************************************/
Int DecodeMacroblockHighpass (CWMImageStrCodec *pSC, CCodingContext *pContext, 
                      Int iMBX, Int iMBY)
{
    /** reset adaptive scan totals **/
    if (pSC->m_bResetRGITotals) {
        JxrAdaptiveScanResetTotals(pContext->m_aScanHoriz, 16);
        JxrAdaptiveScanResetTotals(pContext->m_aScanVert, 16);
    }
    if((pSC->WMISCP.bfBitstreamFormat != SPATIAL) && (pSC->pTile[pSC->cTileColumn].cBitsHP > 0)) { // MB-based HP QP index
        pSC->MBInfo.iQIndexHP = decodeQPIndex(pContext->m_pIOAC, pSC->pTile[pSC->cTileColumn].cBitsHP);
        if (pSC->MBInfo.iQIndexHP >= pSC->pTile[pSC->cTileColumn].cNumQPHP)
            goto ErrorExit;
    }
    else if(pSC->pTile[pSC->cTileColumn].cBitsHP == 0 && pSC->pTile[pSC->cTileColumn].cNumQPHP > 1) // use LP QP
        pSC->MBInfo.iQIndexHP = pSC->MBInfo.iQIndexLP;


    DecodeCBP (pSC, pContext);
    predCBPDec(pSC, pContext);

    if (DecodeCoeffs (pSC, pContext, iMBX, iMBY, 
        pContext->m_pIOAC, pContext->m_pIOFL) != ICERR_OK)
        goto ErrorExit;

    if (pSC->m_bResetContext) {
        AdaptHighpassDec(pContext);
    }

    return ICERR_OK;
ErrorExit:
    return ICERR_ERROR;
}

/*************************************************************************
    Adapt
*************************************************************************/
Int AdaptLowpassDec(CCodingContext * pSC)
{
    Int kk;
    for (kk = 0; kk < CONTEXTX + CTDC; kk++) {
        if (ICERR_OK != AdaptDecFixed (pSC->m_pAHexpt[kk])) {
            goto ErrorExit;
        }
    }
    return ICERR_OK;

ErrorExit:
    return ICERR_ERROR;

}

Int AdaptHighpassDec(CCodingContext * pSC)
{
    Int kk;
    if (ICERR_OK != AdaptDecFixed (pSC->m_pAdaptHuffCBPCY)) {
        goto ErrorExit;
    }
    if (ICERR_OK != AdaptDecFixed (pSC->m_pAdaptHuffCBPCY1)) {
        goto ErrorExit;
    }
    for (kk = 0; kk < CONTEXTX; kk++) {
        if (ICERR_OK != AdaptDecFixed (pSC->m_pAHexpt[kk + CONTEXTX + CTDC])) {
            goto ErrorExit;
        }
    }

    return ICERR_OK;
    
ErrorExit:
    return ICERR_ERROR;
}
