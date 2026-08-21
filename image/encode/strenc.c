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
#include "JXRTrace.h"
#include "encode.h"
#include "strTransform.h"
#include "JxrEncoderMacroblockProcessor.h"
#include "JxrEncoderSubbandPipeline.h"
#include "JxrEncoderPacketHeaderWriter.h"
#include "JxrEncoderSliceFinalizer.h"
#include "JxrEncoderTileHeaderWriter.h"
#include "JxrEncoderImagePlaneHeaderWriter.h"
#include "JxrEncoderMainHeaderWriter.h"
#include "JxrEncoderIndexTableWriter.h"
#include "JxrEncoderPacketStreamAssembler.h"
#include "JxrEncoderPacketStreamCleanup.h"
#include "JxrEncoderPacketStreamInitializer.h"
#include "JxrEncoderQuantizerInitializer.h"
#include <math.h>
#include "perfTimer.h"

#ifdef MEM_TRACE
#define TRACE_MALLOC    1
#define TRACE_NEW       0
#define TRACE_HEAP      0
#include "memtrace.h"
#endif

#ifdef ADI_SYS_OPT
extern char L1WW[];
#endif

#ifdef X86OPT_INLINE
#define _FORCEINLINE __forceinline
#else // X86OPT_INLINE
#define _FORCEINLINE
#endif // X86OPT_INLINE

Int inputMBRow(CWMImageStrCodec *);

#if defined(WMP_OPT_SSE2) || defined(WMP_OPT_CC_ENC) || defined(WMP_OPT_TRFM_ENC)
void StrEncOpt(CWMImageStrCodec* pSC);
#endif // OPT defined


Void writeQuantizer(CWMIQuantizer * pQuantizer[MAX_CHANNELS], BitIOInfo * pIO, U8 cChMode, size_t cChannel, size_t iPos)
{
    JxrEncoderTileHeaderWriterWriteQuantizer(pQuantizer, pIO, cChMode, cChannel, iPos);
}

// packet header: 00000000 00000000 00000001 ?????xxx
// xxx:           000(spatial) 001(DC) 010(AD) 011(AC) 100(FL) 101-111(reserved)
// ?????:         (iTileY * cNumOfSliceV + iTileX)
Void writePacketHeader(BitIOInfo * pIO, U8 ptPacketType, U8 pID)
{
    putBit16(pIO, 0, 8);
    putBit16(pIO, 0, 8);
    putBit16(pIO, 1, 8);
    putBit16(pIO, (pID << 3) + (ptPacketType & 7), 8);
}

Int writeTileHeaderDC(CWMImageStrCodec * pSC, BitIOInfo * pIO)
{
    return JxrEncoderTileHeaderWriterWriteDc(pSC, pIO);
}

Int writeTileHeaderLP(CWMImageStrCodec * pSC, BitIOInfo * pIO)
{
    return JxrEncoderTileHeaderWriterWriteLp(pSC, pIO);
}

Int writeTileHeaderHP(CWMImageStrCodec * pSC, BitIOInfo * pIO)
{
    return JxrEncoderTileHeaderWriterWriteHp(pSC, pIO);
}

Int encodeMB(CWMImageStrCodec * pSC, Int iMBX, Int iMBY)
{
    CCodingContext * pContext = &pSC->m_pCodingContext[pSC->cTileColumn];
    
    JxrEncoderPacketHeaderWriterWrite(pSC, pContext);

    if (JxrEncoderSubbandPipelineProcess(pSC, pContext, iMBX, iMBY) != ICERR_OK)
        return ICERR_ERROR;

    JxrEncoderSliceFinalizerFinalize(pSC, iMBX, iMBY);

    return ICERR_OK;
}

/*************************************************************************
    Top level function for processing a macroblock worth of input
*************************************************************************/
Int processMacroblock(CWMImageStrCodec *pSC)
{
    return JxrEncoderMacroblockProcessorProcess(pSC);
}

/*************************************************************************
  forwardRGBE: forward conversion from RGBE to RGB
*************************************************************************/
static _FORCEINLINE PixelI forwardRGBE (PixelI RGB, PixelI E)
{
    PixelI iResult = 0, iAppend = 1;

    if (E == 0)
        return 0;

    assert (E!=0);

    E--;
    while (((RGB & 0x80) == 0) && (E > 0)) {
        RGB = (RGB << 1) + iAppend;
        iAppend = 0;
        E--;    
    }

    // result will always be one of 3 cases
    // E  RGB       convert to
    // 0  [0.x]      [0   x]
    // 0  [1.x]      [1   x]
    // e  [1.x]      [e+1 x]
    if (E == 0) {
        iResult = RGB;
    }
    else {
        E++;
        iResult = (RGB & 0x7f) + (E << 7);
    }

    return iResult;
}

/*************************************************************************
  convert float-32 into float with (c, lm)!!
*************************************************************************/
static _FORCEINLINE PixelI float2pixel (float f, const char _c, const unsigned char _lm)
{
    union uif
    {
        I32   i;
        float f;
    } x;

    PixelI _h, e, e1, m, s;

    if (f == 0)
    {
        _h = 0;
    }
    else
    {
        x.f = f;

        e = (x.i >> 23) & 0x000000ff;//here set e as e, not s! e includes s: [s e] 9 bits [31..23]
        m = (x.i & 0x007fffff) | 0x800000; // actual mantissa, with normalizer
        if (e == 0) { // denormal-land
            m ^= 0x800000;  // actual mantissa, removing normalizer
            e++; // actual exponent -126
        }

        e1 = e - 127 + _c;  // this is basically a division or quantization to a different exponent
                            // note: _c cannot be greater than 127, so e1 cannot be greater than e
        //assert (_c <= 127);
        if (e1 <= 1) {  // denormal-land
            if (e1 < 1)
                m >>= (1 - e1);  // shift mantissa right to make exponent 1
            e1 = 1;
            if ((m & 0x800000) == 0) // if denormal, set e1 to zero else to 1
                e1 = 0;
        }
        m &= 0x007fffff;

        //for float-22:	    
        _h = (e1 << _lm) + ((m + (1 << (23 - _lm - 1))) >> (23 - _lm));//take 23-bit m, shift (23-lm), get lm-bit m for float22
        s = ((PixelI) x.i) >> 31;
        //padding to int-32: 
        _h = (_h ^ s) - s;	
    }

    return _h;
}

/*************************************************************************
  convert Half-16 to internal format, only need to handle sign bit
*************************************************************************/
static _FORCEINLINE PixelI forwardHalf (PixelI hHalf)
{
    PixelI s;
    s = hHalf >> 31;
    hHalf = ((hHalf & 0x7fff) ^ s) - s;
    return hHalf;
}


//================================================================
// Color Conversion 
// functions to get image data from input buffer
// this inlcudes necessary color conversion and boundary padding
//================================================================
#define _CC(r, g, b) (b -= r, r += ((b + 1) >> 1) - g, g += ((r + 0) >> 1))
#define _CC_CMYK(c, m, y, k) (y -= c, c += ((y + 1) >> 1) - m, m += (c >> 1) - k, k += ((m + 1) >> 1))

//================================================================
// BitIOInfo init/term for encoding

Int StrIOEncInit(CWMImageStrCodec* pSC)
{
    return JxrEncoderPacketStreamInitializerInitialize(pSC);
}

Int writeIndexTableNull(CWMImageStrCodec * pSC)
{
    return JxrEncoderIndexTableWriterWriteNull(pSC);
}

Int writeIndexTable(CWMImageStrCodec * pSC)
{
    return JxrEncoderIndexTableWriterWrite(pSC);
}

Int StrIOEncTerm(CWMImageStrCodec* pSC)
{
    BitIOInfo * pIO = pSC->pIOHeader;

    fillToByte(pIO);

    if(pSC->WMISCP.bVerbose){
        U32 i, j;

        printf("\n%d horizontal tiles:\n", pSC->WMISCP.cNumOfSliceMinus1H + 1);
        for(i = 0; i <= pSC->WMISCP.cNumOfSliceMinus1H; i ++){
            printf("    offset of tile %d in MBs: %d\n", i, pSC->WMISCP.uiTileY[i]);
        }

        printf("\n%d vertical tiles:\n", pSC->WMISCP.cNumOfSliceMinus1V + 1);
        for(i = 0; i <= pSC->WMISCP.cNumOfSliceMinus1V; i ++){
            printf("    offset of tile %d in MBs: %d\n", i, pSC->WMISCP.uiTileX[i]);
        }

        if(pSC->WMISCP.bfBitstreamFormat == SPATIAL){
            printf("\nSpatial order bitstream\n");
        }
        else{
            printf("\nFrequency order bitstream\n");
        }

        if(!pSC->m_param.bIndexTable){
            printf("\nstreaming mode, no index table.\n");
        }
        else if(pSC->WMISCP.bfBitstreamFormat == SPATIAL){
            for(j = 0; j <= pSC->WMISCP.cNumOfSliceMinus1H; j ++){
                for(i = 0; i <= pSC->WMISCP.cNumOfSliceMinus1V; i ++){
                    printf("bitstream size for tile (%d, %d): %d.\n", j, i, (int) pSC->pIndexTable[j * (pSC->WMISCP.cNumOfSliceMinus1V + 1) + i]);
                }
            }
        }
        else{
            for(j = 0; j <= pSC->WMISCP.cNumOfSliceMinus1H; j ++){
                for(i = 0; i <= pSC->WMISCP.cNumOfSliceMinus1V; i ++){
                    size_t * p = &pSC->pIndexTable[(j * (pSC->WMISCP.cNumOfSliceMinus1V + 1) + i) * 4];
                    printf("bitstream size of (DC, LP, AC, FL) for tile (%d, %d): %d %d %d %d.\n", j, i,
                        (int) p[0], (int) p[1], (int) p[2], (int) p[3]);
                }
            }
        }
    }
    
    writeIndexTable(pSC); // write index table to the header

    detachISWrite(pSC, pIO);

    if(pSC->cNumBitIO > 0){
        JxrEncoderPacketStreamAssemblerAssemble(pSC);

        if (JxrEncoderPacketStreamCleanupRelease(pSC) != ICERR_OK)
            return ICERR_ERROR;
    }

    return 0;
}

/*************************************************************************
    Write header of image plane
*************************************************************************/
Int WriteImagePlaneHeader(CWMImageStrCodec * pSC)
{
    return JxrEncoderImagePlaneHeaderWriterWrite(pSC);
}

Int WriteWMIHeader(CWMImageStrCodec * pSC)
{
    return JxrEncoderMainHeaderWriterWrite(pSC);
}

Int StrEncInit(CWMImageStrCodec* pSC)
{
    COLORFORMAT cf = pSC->m_param.cfColorFormat;
    COLORFORMAT cfE = pSC->WMII.cfColorFormat;
    Bool b32bit = sizeof(size_t) == 4;

    /** color transcoding with resolution change **/
    pSC->m_bUVResolutionChange = (((cfE == CF_RGB || cfE == YUV_444 || cfE == CMYK || cfE == CF_RGBE) && 
								   (cf == YUV_422 || cf == YUV_420))
								  || (cfE == YUV_422 && cf == YUV_420)) && !pSC->WMISCP.bYUVData;

    if(pSC->m_bUVResolutionChange){
        size_t cSize = ((cfE == YUV_422 ? 128 : 256) + (cf == YUV_420 ? 32 : 0)) * pSC->cmbWidth + 256;

        if(b32bit){ // integer overlow/underflow check for 32-bit system
            if(((pSC->cmbWidth >> 16) * ((cfE == YUV_422 ? 128 : 256) + (cf == YUV_420 ? 32 : 0))) & 0xffff0000)
                return ICERR_ERROR;
            if(cSize >= 0x3fffffff)
                return ICERR_ERROR;
        }
        pSC->pResU = (PixelI *)malloc(cSize * sizeof(PixelI));
        pSC->pResV = (PixelI *)malloc(cSize * sizeof(PixelI));
        if(pSC->pResU == NULL || pSC->pResV == NULL){
            return ICERR_ERROR;
        }
    }

    pSC->cTileColumn = pSC->cTileRow = 0;

    if(allocateTileInfo(pSC) != ICERR_OK)
        return ICERR_ERROR;

    if (JxrEncoderQuantizerInitializerInitialize(pSC) != ICERR_OK)
        return ICERR_ERROR;

    if(allocatePredInfo(pSC) != ICERR_OK){
        return ICERR_ERROR;
    }

    if(pSC->WMISCP.cNumOfSliceMinus1V >= MAX_TILES || AllocateCodingContextEnc (pSC, pSC->WMISCP.cNumOfSliceMinus1V + 1, pSC->WMISCP.uiTrimFlexBits) != ICERR_OK){
        return ICERR_ERROR;
    }
    
    if (pSC->m_bSecondary) {
        pSC->pIOHeader = pSC->m_pNextSC->pIOHeader;
        pSC->m_ppBitIO = pSC->m_pNextSC->m_ppBitIO;
        pSC->cNumBitIO = pSC->m_pNextSC->cNumBitIO;
        pSC->cSB = pSC->m_pNextSC->cSB;
        pSC->ppWStream = pSC->m_pNextSC->ppWStream;
        pSC->pIndexTable = pSC->m_pNextSC->pIndexTable;
        setBitIOPointers(pSC);
    }
    else {
        StrIOEncInit(pSC);
        setBitIOPointers(pSC);
        WriteWMIHeader(pSC);
    }

    return ICERR_OK;
}

static Int StrEncTerm(CTXSTRCODEC ctxSC)
{
    CWMImageStrCodec* pSC = (CWMImageStrCodec*)ctxSC;
    JXRTraceDumpCodecState("encoder", pSC);
    size_t j, jend = (pSC->m_pNextSC != NULL);

    for (j = 0; j <= jend; j++) {
        if (sizeof(*pSC) != pSC->cbStruct) {
            return ICERR_ERROR;
        }

        if(pSC->m_bUVResolutionChange){
            if(pSC->pResU != NULL)
                free(pSC->pResU);
            if(pSC->pResV != NULL)
                free(pSC->pResV);
        }

        freePredInfo(pSC);

        if (j == 0)
            StrIOEncTerm(pSC);

        FreeCodingContextEnc(pSC);
        
        freeTileInfo(pSC);

        pSC->WMISCP.nExpBias -= 128; // reset

        pSC = pSC->m_pNextSC;
    }

    return 0;
}

U32 setUniformTiling(U32 * pTile, U32 cNumTile, U32 cNumMB)
{
    U32 i, j;

    while((cNumMB + cNumTile - 1) / cNumTile > 65535) // too few tiles
        cNumTile ++;

    for(i = cNumTile, j = cNumMB; i > 1; i --){
        pTile[cNumTile - i] = (j + i - 1) / i;
        j -= pTile[cNumTile - i];
    }

    return cNumTile;
}

U32 validateTiling(U32 * pTile, U32 cNumTile, U32 cNumMB)
{
    U32 i, cMBs;

    if(cNumTile == 0)
        cNumTile = 1;
    if(cNumTile > cNumMB) // too many tiles
        cNumTile = 1;
    if(cNumTile > MAX_TILES)
        cNumTile = MAX_TILES;

    for(i = cMBs = 0; i + 1 < cNumTile; i ++){
        if(pTile[i] == 0 || pTile[i] > 65535){ // invalid tile setting, resetting to uniform tiling
            cNumTile = setUniformTiling(pTile, cNumTile, cNumMB);
            break;
        }
        
        cMBs += pTile[i];

        if(cMBs >= cNumMB){
            cNumTile = i + 1;
            break;
        }
    }

    // last tile
    if(cNumMB - cMBs > 65536)
        cNumTile = setUniformTiling(pTile, cNumTile, cNumMB);

    for(i = 1; i < cNumTile; i ++)
        pTile[i] += pTile[i - 1];
    for(i = cNumTile - 1; i > 0; i --)
        pTile[i] = pTile[i - 1];
    pTile[0] = 0;

    return cNumTile;
}

/*************************************************************************
  Validate and adjust input params here
*************************************************************************/
Int ValidateArgs(CWMImageInfo* pII, CWMIStrCodecParam *pSCP)
{
    int i;
    Bool bTooNarrowTile = FALSE;

    if(pII->cWidth > (1 << 28) || pII->cHeight > (1 << 28) || pII->cWidth == 0 || pII->cHeight == 0){
        printf("Unsurpported image size!\n");
        return ICERR_ERROR; // unsurpported image size
    }

    if (((pSCP->cfColorFormat == YUV_420) || (pSCP->cfColorFormat == YUV_422)) && (pSCP->olOverlap == OL_TWO) && ((Int)(((U32)pII->cWidth + 15) >> 4) < 2)) {
        printf("Image width must be at least 2 MB wide for subsampled chroma and two levels of overlap!\n");
        return ICERR_ERROR;
    }

    if(pSCP->sbSubband == SB_ISOLATED || pSCP->sbSubband >= SB_MAX) // not allowed
        pSCP->sbSubband = SB_ALL;

    if(pII->bdBitDepth == BD_5 && (pII->cfColorFormat != CF_RGB || pII->cBitsPerUnit != 16 || pII->cLeadingPadding != 0)){
        printf("Unsupported BD_5 image format!\n");
        return ICERR_ERROR; // BD_5 must be compact RGB!
    }   
    if(pII->bdBitDepth == BD_565 && (pII->cfColorFormat != CF_RGB || pII->cBitsPerUnit != 16 || pII->cLeadingPadding != 0)){
        printf("Unsupported BD_565 image format!\n");
        return ICERR_ERROR; // BD_5 must be compact RGB!
    }   
    if(pII->bdBitDepth == BD_10 && (pII->cfColorFormat != CF_RGB || pII->cBitsPerUnit != 32 || pII->cLeadingPadding != 0)){
        printf("Unsupported BD_10 image format!\n");
        return ICERR_ERROR; // BD_10 must be compact RGB!
    }

    if((pII->bdBitDepth == BD_5 || pII->bdBitDepth == BD_565 || pII->bdBitDepth == BD_10) && 
        (pSCP->cfColorFormat != YUV_420 && pSCP->cfColorFormat != YUV_422 && pSCP->cfColorFormat != Y_ONLY))
            pSCP->cfColorFormat = YUV_444;

    if(BD_1 == pII->bdBitDepth){ // binary image
        if(pII->cfColorFormat != Y_ONLY){
            printf("BD_1 image must be black-and white!\n");
            return ICERR_ERROR;
        }
        pSCP->cfColorFormat = Y_ONLY; // can only be black white
    }

    if(pSCP->bdBitDepth != BD_LONG)
        pSCP->bdBitDepth = BD_LONG; // currently only support 32 bit internally

    if(pSCP->uAlphaMode > 1 && (pII->cfColorFormat == YUV_420 || pII->cfColorFormat == YUV_422 
								|| pII->bdBitDepth == BD_5 || pII->bdBitDepth == BD_10 
								|| pII->bdBitDepth == BD_1))
    {
        printf("Alpha is not supported for this pixel format!\n");
        return ICERR_ERROR;
    }

    if((pSCP->cfColorFormat == YUV_420 || pSCP->cfColorFormat == YUV_422) && (pII->bdBitDepth == BD_16F || pII->bdBitDepth == BD_32F || pII->cfColorFormat == CF_RGBE))
    {
        printf("Float or RGBE images must be encoded with YUV 444!\n");
        return ICERR_ERROR;
    }

    // adjust tiling
    pSCP->cNumOfSliceMinus1V = validateTiling(pSCP->uiTileX, pSCP->cNumOfSliceMinus1V + 1, (((U32)pII->cWidth + 15) >> 4)) - 1;
    pSCP->cNumOfSliceMinus1H = validateTiling(pSCP->uiTileY, pSCP->cNumOfSliceMinus1H + 1, (((U32)pII->cHeight + 15) >> 4)) - 1;

    if (pSCP->bUseHardTileBoundaries && ((pSCP->cfColorFormat == YUV_420) || (pSCP->cfColorFormat == YUV_422)) && (pSCP->olOverlap == OL_TWO)) {
        for (i = 1; i < (int) (pSCP->cNumOfSliceMinus1H + 1); i++) {
            if ((Int)(pSCP->uiTileY[i] - pSCP->uiTileY[i - 1]) < 2) {
                bTooNarrowTile = TRUE;
                break;
            }
        }
        if ((Int)((((U32)pII->cWidth + 15) >> 4) - pSCP->uiTileY[pSCP->cNumOfSliceMinus1H]) < 2) 
            bTooNarrowTile = TRUE;
    }
    if (bTooNarrowTile) {
        printf("Tile width must be at least 2 MB wide for hard tiles, subsampled chroma, and two levels of overlap!\n");
        return ICERR_ERROR;
    }

    if(pSCP->cChannel > MAX_CHANNELS)
        return ICERR_ERROR;

    /** supported color transcoding **/
    /** ARGB, RGB => YUV_444, YUV_422, YUV_420, Y_ONLY **/
    /** YUV_444   =>          YUV_422, YUV_420, Y_ONLY **/
    /** YUV_422   =>                   YUV_420, Y_ONLY **/
    /** YUV_420   =>                            Y_ONLY **/

    /** unsupported color transcoding       **/
    /** Y_ONLY, YUV_420, YUV_422 => YUV_444 **/
    /** Y_ONLY, YUV_420          => YUV_422 **/
    /** Y_ONLY                   => YUV_420 **/
    if((pII->cfColorFormat == Y_ONLY &&  pSCP->cfColorFormat != Y_ONLY) || 
        (pSCP->cfColorFormat == YUV_422 && (pII->cfColorFormat == YUV_420 || pII->cfColorFormat == Y_ONLY)) || 
        (pSCP->cfColorFormat == YUV_444 && (pII->cfColorFormat == YUV_422 || pII->cfColorFormat == YUV_420 || pII->cfColorFormat == Y_ONLY))){
		pSCP->cfColorFormat = pII->cfColorFormat; // force not to do color transcoding!
    }
    else if (pII->cfColorFormat == NCOMPONENT) {
		pSCP->cfColorFormat = NCOMPONENT; // force not to do color transcoding!
    }
    if (CMYK == pII->cfColorFormat && pSCP->cfColorFormat == NCOMPONENT) 
    {
        pSCP->cfColorFormat = CMYK;
    }

    if(pSCP->cfColorFormat != NCOMPONENT){
        if(pSCP->cfColorFormat == Y_ONLY)
            pSCP->cChannel = 1;
        else if(pSCP->cfColorFormat == CMYK)
            pSCP->cChannel = 4;
        else
            pSCP->cChannel = 3;
    }

    if(pSCP->sbSubband >= SB_MAX)
        pSCP->sbSubband = SB_ALL;


    pII->cChromaCenteringX = 0;
    pII->cChromaCenteringY = 0;

    return ICERR_OK;
}

/*************************************************************************
  Initialization of CWMImageStrCodec struct
*************************************************************************/
static Void InitializeStrEnc(CWMImageStrCodec *pSC,
    const CWMImageInfo* pII, const CWMIStrCodecParam *pSCP)
{
    pSC->cbStruct = sizeof(*pSC);
    pSC->WMII = *pII;
    pSC->WMISCP = *pSCP;

    // set nExpBias
    if (pSC->WMISCP.nExpBias == 0)
        pSC->WMISCP.nExpBias = 4 + 128;//default
    pSC->WMISCP.nExpBias += 128; // rollover arithmetic

    pSC->cRow = 0;
    pSC->cColumn = 0;
    
    pSC->cmbWidth = (pSC->WMII.cWidth + 15) / 16;
    pSC->cmbHeight = (pSC->WMII.cHeight + 15) / 16;

    pSC->Load = inputMBRow;
    pSC->Quantize = quantizeMacroblock;
    pSC->ProcessTopLeft = processMacroblock;
    pSC->ProcessTop = processMacroblock;
    pSC->ProcessTopRight = processMacroblock;
    pSC->ProcessLeft = processMacroblock;
    pSC->ProcessCenter = processMacroblock;
    pSC->ProcessRight = processMacroblock;
    pSC->ProcessBottomLeft = processMacroblock;
    pSC->ProcessBottom = processMacroblock;
    pSC->ProcessBottomRight = processMacroblock;

    pSC->m_pNextSC = NULL;
    pSC->m_bSecondary = FALSE;
}

/*************************************************************************
   Streaming API init
*************************************************************************/
Int ImageStrEncInit(
    CWMImageInfo* pII,
    CWMIStrCodecParam *pSCP,
    CTXSTRCODEC* pctxSC)
{
    static size_t cbChannels[BD_MAX] = {2, 4};

    size_t cbChannel = 0, cblkChroma = 0, i;
    size_t cbMacBlockStride = 0, cbMacBlockChroma = 0, cMacBlock = 0;

    CWMImageStrCodec* pSC = NULL, *pNextSC = NULL;
    char* pb = NULL;
    size_t cb = 0;
    Bool b32bit = sizeof(size_t) == 4;

    Int err;

    if(ValidateArgs(pII, pSCP) != ICERR_OK){
        goto ErrorExit;
    }

    //================================================
    *pctxSC = NULL;

    //================================================
    cbChannel = cbChannels[pSCP->bdBitDepth];
    cblkChroma = cblkChromas[pSCP->cfColorFormat];
    cbMacBlockStride = cbChannel * 16 * 16;
    cbMacBlockChroma = cbChannel * 16 * cblkChroma;
    cMacBlock = (pII->cWidth + 15) / 16;

    //================================================
    cb = sizeof(*pSC) + (128 - 1) + (PACKETLENGTH * 4 - 1) + (PACKETLENGTH * 2 ) + sizeof(*pSC->pIOHeader);
    i = cbMacBlockStride + cbMacBlockChroma * (pSCP->cChannel - 1);
    if(b32bit) // integer overlow/underflow check for 32-bit system
        if(((cMacBlock >> 15) * i) & 0xffff0000)
            return ICERR_ERROR;
    i *= cMacBlock * 2;
    cb += i;

    pb = malloc(cb);
    if (NULL == pb)
    {
        goto ErrorExit;
    }
    memset(pb, 0, cb);

    //================================================
    pSC = (CWMImageStrCodec*)pb; pb += sizeof(*pSC);

    // Set up perf timers
    PERFTIMER_ONLY(pSC->m_fMeasurePerf = pSCP->fMeasurePerf);
    PERFTIMER_NEW(pSC->m_fMeasurePerf, &pSC->m_ptEndToEndPerf);
    PERFTIMER_NEW(pSC->m_fMeasurePerf, &pSC->m_ptEncDecPerf);
    PERFTIMER_START(pSC->m_fMeasurePerf, pSC->m_ptEndToEndPerf);
    PERFTIMER_START(pSC->m_fMeasurePerf, pSC->m_ptEncDecPerf);
    PERFTIMER_COPYSTARTTIME(pSC->m_fMeasurePerf, pSC->m_ptEncDecPerf, pSC->m_ptEndToEndPerf);

    pSC->m_param.cfColorFormat = pSCP->cfColorFormat;
    pSC->m_param.bAlphaChannel = (pSCP->uAlphaMode == 3);
    pSC->m_param.cNumChannels = pSCP->cChannel;
    pSC->m_param.cExtraPixelsTop = pSC->m_param.cExtraPixelsBottom
        = pSC->m_param.cExtraPixelsLeft = pSC->m_param.cExtraPixelsRight = 0;

    pSC->cbChannel = cbChannel;

    pSC->m_param.bTranscode = pSC->bTileExtraction = FALSE;

    //================================================
    InitializeStrEnc(pSC, pII, pSCP);

    //================================================
    // 2 Macro Row buffers for each channel
    pb = ALIGNUP(pb, 128);
    for (i = 0; i < pSC->m_param.cNumChannels; i++) {
        pSC->a0MBbuffer[i] = (PixelI*)pb; pb += cbMacBlockStride * pSC->cmbWidth;
        pSC->a1MBbuffer[i] = (PixelI*)pb; pb += cbMacBlockStride * pSC->cmbWidth;
        cbMacBlockStride = cbMacBlockChroma;
    }

    //================================================
    // lay 2 aligned IO buffers just below pIO struct
    pb = (char*)ALIGNUP(pb, PACKETLENGTH * 4) + PACKETLENGTH * 2;
    pSC->pIOHeader = (BitIOInfo*)pb;

    //================================================
    err = StrEncInit(pSC);
    if (ICERR_OK != err)
        goto ErrorExit;

    // if interleaved alpha is needed
    if (pSC->m_param.bAlphaChannel) {
        cbMacBlockStride = cbChannel * 16 * 16;
        // 1. allocate new pNextSC info
        //================================================
        cb = sizeof(*pNextSC) + (128 - 1) + cbMacBlockStride * cMacBlock * 2;
        pb = malloc(cb);
        if (NULL == pb)
        {
            goto ErrorExit;
        }
        memset(pb, 0, cb);
        //================================================
        pNextSC = (CWMImageStrCodec*)pb; pb += sizeof(*pNextSC);

        // 2. initialize pNextSC
        pNextSC->m_param.cfColorFormat = Y_ONLY;
        pNextSC->m_param.cNumChannels = 1;
        pNextSC->m_param.bAlphaChannel = TRUE;
        pNextSC->cbChannel = cbChannel;
        //================================================

        // 3. initialize arrays
        InitializeStrEnc(pNextSC, pII, pSCP);
        //================================================

        // 2 Macro Row buffers for each channel
        pb = ALIGNUP(pb, 128);
        pNextSC->a0MBbuffer[0] = (PixelI*)pb; pb += cbMacBlockStride * pNextSC->cmbWidth;
        pNextSC->a1MBbuffer[0] = (PixelI*)pb; pb += cbMacBlockStride * pNextSC->cmbWidth;
        //================================================
        pNextSC->pIOHeader = pSC->pIOHeader;
        //================================================

        // 4. link pSC->pNextSC = pNextSC
        pNextSC->m_pNextSC = pSC;
        pNextSC->m_bSecondary = TRUE;

        // 5. StrEncInit
        StrEncInit(pNextSC);

        // 6. Write header of image plane
        WriteImagePlaneHeader(pNextSC);
    }

    pSC->m_pNextSC = pNextSC;
    //================================================
    *pctxSC = (CTXSTRCODEC)pSC;

    writeIndexTableNull(pSC);
#if defined(WMP_OPT_SSE2) || defined(WMP_OPT_CC_ENC) || defined(WMP_OPT_TRFM_ENC)
    StrEncOpt(pSC);
#endif // OPT defined

    PERFTIMER_STOP(pSC->m_fMeasurePerf, pSC->m_ptEncDecPerf);
    return ICERR_OK;

ErrorExit:
    return ICERR_ERROR;
}

/*************************************************************************
   Streaming API encode
*************************************************************************/
Int ImageStrEncEncode(
    CTXSTRCODEC ctxSC,
    const CWMImageBufferInfo* pBI)
{
    CWMImageStrCodec* pSC = (CWMImageStrCodec*)ctxSC;
    JXRTraceDumpCodecState("encoder", pSC);
    CWMImageStrCodec* pNextSC = pSC->m_pNextSC;
    ImageDataProc ProcessLeft, ProcessCenter, ProcessRight;

    if (sizeof(*pSC) != pSC->cbStruct)
    {
        return ICERR_ERROR;
    }

    //================================
    PERFTIMER_START(pSC->m_fMeasurePerf, pSC->m_ptEncDecPerf);

    pSC->WMIBI = *pBI;
    pSC->cColumn = 0;
    initMRPtr(pSC);
    if (pNextSC)
        pNextSC->WMIBI = *pBI;

    if (0 == pSC->cRow) {
        ProcessLeft = pSC->ProcessTopLeft;
        ProcessCenter = pSC->ProcessTop;
        ProcessRight = pSC->ProcessTopRight;
    }
    else {
        ProcessLeft = pSC->ProcessLeft;
        ProcessCenter = pSC->ProcessCenter;
        ProcessRight = pSC->ProcessRight;
    }

    if( pSC->Load(pSC) != ICERR_OK )
		return ICERR_ERROR;
    JXRTraceDumpStage("encoder", "centered_samples", pSC, 0, (Int)pSC->cRow, JXRTraceSamples);
    if(ProcessLeft(pSC) != ICERR_OK)
        return ICERR_ERROR;
    advanceMRPtr(pSC);

    //================================
    for (pSC->cColumn = 1; pSC->cColumn < pSC->cmbWidth; ++pSC->cColumn) {
        if(ProcessCenter(pSC) != ICERR_OK)
            return ICERR_ERROR;
        advanceMRPtr(pSC);
    }

    //================================
    if(ProcessRight(pSC) != ICERR_OK)
        return ICERR_ERROR;
    if (pSC->cRow)
        advanceOneMBRow(pSC);

    ++pSC->cRow;
    swapMRPtr(pSC);

    PERFTIMER_STOP(pSC->m_fMeasurePerf, pSC->m_ptEncDecPerf);
    return ICERR_OK;
}

/*************************************************************************
   Streaming API term
*************************************************************************/
Int ImageStrEncTerm(
    CTXSTRCODEC ctxSC)
{
    CWMImageStrCodec* pSC = (CWMImageStrCodec*)ctxSC;
    JXRTraceDumpCodecState("encoder", pSC);
    // CWMImageStrCodec *pNextSC = pSC->m_pNextSC;

    if (sizeof(*pSC) != pSC->cbStruct)
    {
        return ICERR_ERROR;
    }

    //================================
    PERFTIMER_START(pSC->m_fMeasurePerf, pSC->m_ptEncDecPerf);
    pSC->cColumn = 0;
    initMRPtr(pSC);

    pSC->ProcessBottomLeft(pSC);
    advanceMRPtr(pSC);

    //================================
    for (pSC->cColumn = 1; pSC->cColumn < pSC->cmbWidth; ++pSC->cColumn) {
        pSC->ProcessBottom(pSC);
        advanceMRPtr(pSC);
    }

    //================================
    pSC->ProcessBottomRight(pSC);

    //================================
    StrEncTerm(pSC);

    PERFTIMER_STOP(pSC->m_fMeasurePerf, pSC->m_ptEncDecPerf);
    PERFTIMER_STOP(pSC->m_fMeasurePerf, pSC->m_ptEndToEndPerf);
    PERFTIMER_REPORT(pSC->m_fMeasurePerf, pSC);
    PERFTIMER_DELETE(pSC->m_fMeasurePerf, pSC->m_ptEncDecPerf);
    PERFTIMER_DELETE(pSC->m_fMeasurePerf, pSC->m_ptEndToEndPerf);

    free(pSC);
    return ICERR_OK;
}

// centralized UV downsampling
#define DF_ODD ((((d1 + d2 + d3) << 2) + (d2 << 1) + d0 + d4 + 8) >> 4)
Void downsampleUV(CWMImageStrCodec * pSC)
{
    const COLORFORMAT cfInt = pSC->m_param.cfColorFormat;
    const COLORFORMAT cfExt = pSC->WMII.cfColorFormat;
    PixelI * pSrc, * pDst;
    PixelI d0, d1, d2, d3, d4;
    size_t iChannel, iRow, iColumn;

    for(iChannel = 1; iChannel < 3; iChannel ++){
        if(cfExt != YUV_422){ // need to do horizontal downsampling, 444 => 422
            const size_t cShift = (cfInt == YUV_422 ? 1 : 0);

            pSrc = (iChannel == 1 ? pSC->pResU : pSC->pResV);
            pDst = (cfInt == YUV_422 ? pSC->p1MBbuffer[iChannel] : pSrc);
            
            for(iRow = 0; iRow < 16; iRow ++){
                d0 = d4 = pSrc[idxCC[iRow][2]], d1 = d3 = pSrc[idxCC[iRow][1]], d2 = pSrc[idxCC[iRow][0]]; // left boundary
                
                for(iColumn = 0; iColumn + 2 < pSC->cmbWidth * 16; iColumn += 2){
                    pDst[((iColumn >> 4) << (8 - cShift)) + idxCC[iRow][(iColumn & 15) >> cShift]] = DF_ODD;
                    d0 = d2, d1 = d3, d2 = d4;
                    d3 = pSrc[(((iColumn + 3) >> 4) << 8) + idxCC[iRow][(iColumn + 3) & 0xf]];
                    d4 = pSrc[(((iColumn + 4) >> 4) << 8) + idxCC[iRow][(iColumn + 4) & 0xf]];
                }

                d4 = d2; // right boundary
                pDst[((iColumn >> 4) << (8 - cShift)) + idxCC[iRow][(iColumn & 15) >> cShift]] = DF_ODD;
            }
        }

        if(cfInt == YUV_420){ // need to do vertical downsampling
            const size_t cShift = (cfExt == YUV_422 ? 0 : 1);
            PixelI * pBuf[4];
            size_t mbOff, pxOff;
            
            pDst = pSC->p1MBbuffer[iChannel];
            pSrc = (iChannel == 1 ? pSC->pResU : pSC->pResV);
            pBuf[0] = pSrc + (pSC->cmbWidth << (cfExt == YUV_422 ? 7 : 8));
            pBuf[1] = pBuf[0] + pSC->cmbWidth * 8, pBuf[2] = pBuf[1] + pSC->cmbWidth * 8, pBuf[3] = pBuf[2] + pSC->cmbWidth * 8;

            for(iColumn = 0; iColumn < pSC->cmbWidth * 8; iColumn ++){
                mbOff = (iColumn >> 3) << (7 + cShift);
                pxOff = (iColumn & 7) << cShift;

                if(pSC->cRow == 0) // top image boundary
                    d0 = d4 = pSrc[mbOff + idxCC[2][pxOff]], d1 = d3 = pSrc[mbOff + idxCC[1][pxOff]], d2 = pSrc[mbOff + idxCC[0][pxOff]]; // top MB boundary
                else{
                    // last row of previous MB row
                    d0 = pBuf[0][iColumn], d1 = pBuf[1][iColumn], d2 = pBuf[2][iColumn], d3 = pBuf[3][iColumn], d4 = pSrc[mbOff + idxCC[0][pxOff]];
                    pSC->p0MBbuffer[iChannel][((iColumn >> 3) << 6) + idxCC_420[7][iColumn & 7]] = DF_ODD;

                    // for first row of current MB
                    d0 = pBuf[2][iColumn], d1 = pBuf[3][iColumn];
                    d2 = pSrc[mbOff + idxCC[0][pxOff]], d3 = pSrc[mbOff + idxCC[1][pxOff]], d4 = pSrc[mbOff + idxCC[2][pxOff]];
                }

                for(iRow = 0; iRow < 12; iRow += 2){
                    pDst[((iColumn >> 3) << 6) + idxCC_420[iRow >> 1][iColumn & 7]] = DF_ODD;
                    d0 = d2, d1 = d3, d2 = d4;
                    d3 = pSrc[mbOff + idxCC[iRow + 3][pxOff]];
                    d4 = pSrc[mbOff + idxCC[iRow + 4][pxOff]];
                }
                
                //last row of current MB
                pDst[((iColumn >> 3) << 6) + idxCC_420[6][iColumn & 7]] = DF_ODD;
                d0 = d2, d1 = d3, d2 = d4;
                d3 = pSrc[mbOff + idxCC[iRow + 3][pxOff]];

                if(pSC->cRow + 1 == pSC->cmbHeight){ // bottom image boundary
                    d4 = d2;
                    pDst[((iColumn >> 3) << 6) + idxCC_420[7][iColumn & 7]] = DF_ODD;
                }
                else{
                    for(iRow = 0; iRow < 4; iRow ++)
                        pBuf[iRow][iColumn] = pSrc[mbOff + idxCC[iRow + 12][pxOff]];
                }
            }
        }
    }
}

// centralized horizontal padding
Void padHorizontally(CWMImageStrCodec * pSC)
{
    if(pSC->WMII.cWidth != pSC->cmbWidth * 16){ // horizontal padding is necessary!
        const COLORFORMAT cfExt = pSC->WMISCP.bYUVData ?
            pSC->m_param.cfColorFormat : pSC->WMII.cfColorFormat;
        size_t cFullChannel = pSC->WMISCP.cChannel;
        size_t iLast = pSC->WMII.cWidth - 1;
        PixelI * pCh[16];
        size_t iChannel, iColumn, iRow;

        if(cfExt == YUV_420 || cfExt == YUV_422 || cfExt == Y_ONLY)
            cFullChannel = 1;

        assert(cFullChannel <= 16);

        assert(pSC->WMISCP.cChannel <= 16);
        for(iChannel = 0; iChannel < pSC->WMISCP.cChannel; iChannel ++)
            pCh[iChannel & 15] = pSC->p1MBbuffer[iChannel & 15];

        if(pSC->m_bUVResolutionChange)
            pCh[1] = pSC->pResU, pCh[2] = pSC->pResV;

        // pad full resoluton channels
        for(iRow = 0; iRow < 16; iRow ++){
            const size_t iPosLast = ((iLast >> 4) << 8) + idxCC[iRow][iLast & 0xf];
            for(iColumn = iLast + 1; iColumn < pSC->cmbWidth * 16; iColumn ++){
                const size_t iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
                for(iChannel = 0; iChannel < cFullChannel; iChannel ++)
                    pCh[iChannel & 15][iPos] = pCh[iChannel & 15][iPosLast];
            }
        }

        if(cfExt == YUV_422) // pad YUV_422 UV
            for(iLast >>= 1, iRow = 0; iRow < 16; iRow ++){
                const size_t iPosLast = ((iLast >> 3) << 7) + idxCC[iRow][iLast & 7];
                for(iColumn = iLast + 1; iColumn < pSC->cmbWidth * 8; iColumn ++){
                    const size_t iPos = ((iColumn >> 3) << 7) + idxCC[iRow][iColumn & 7];
                    for(iChannel = 1; iChannel < 3; iChannel ++)
                        pCh[iChannel][iPos] = pCh[iChannel][iPosLast];
                }
            }
        else if(cfExt == YUV_420) // pad YUV_420 UV
            for(iLast >>= 1, iRow = 0; iRow < 8; iRow ++){
                const size_t iPosLast = ((iLast >> 3) << 6) + idxCC_420[iRow][iLast & 7];
                for(iColumn = iLast + 1; iColumn < pSC->cmbWidth * 8; iColumn ++){
                    const size_t iPos = ((iColumn >> 3) << 6) + idxCC_420[iRow][iColumn & 7];
                    for(iChannel = 1; iChannel < 3; iChannel ++)
                        pCh[iChannel][iPos] = pCh[iChannel][iPosLast];
                }
            }
    }
}

// centralized alpha channel color conversion, small perf penalty
Int inputMBRowAlpha(CWMImageStrCodec* pSC)
{
    if(pSC->m_bSecondary == FALSE && pSC->m_pNextSC != NULL){ // alpha channel is present
        const size_t cShift = (pSC->m_pNextSC->m_param.bScaledArith ? (SHIFTZERO + QPFRACBITS) : 0);
        const BITDEPTH_BITS bdExt = pSC->WMII.bdBitDepth;
        const size_t iAlphaPos = pSC->WMII.cLeadingPadding + (pSC->WMII.cfColorFormat == CMYK ? 4 : 3);//only RGB and CMYK may have interleaved alpha
        const size_t cRow = pSC->WMIBI.cLine;
        const size_t cColumn = pSC->WMII.cWidth;
        const U8 * pSrc0 = (U8 *)pSC->WMIBI.pv;
        PixelI * pA = pSC->m_pNextSC->p1MBbuffer[0];
        size_t iRow, iColumn;

        for(iRow = 0; iRow < 16; iRow ++){
            if(bdExt == BD_8){
                const size_t cStride = (pSC->WMII.cBitsPerUnit >> 3);
                const U8 * pSrc = pSrc0;
                
                for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cStride)
                    pA[((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf]] = ((PixelI)pSrc[iAlphaPos] - (1 << 7)) << cShift;
            }
            else if(bdExt == BD_16){
                const size_t cStride = (pSC->WMII.cBitsPerUnit >> 3) / sizeof(U16);
                const U8 nLenMantissaOrShift = pSC->m_pNextSC->WMISCP.nLenMantissaOrShift;
                const U16 * pSrc = (U16 *)pSrc0;

                for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cStride)
                    pA[((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf]] = ((((PixelI)pSrc[iAlphaPos] - (1 << 15)) >> nLenMantissaOrShift) << cShift);
            }
            else if(bdExt == BD_16S){
                const size_t cStride = (pSC->WMII.cBitsPerUnit >> 3) / sizeof(I16);
                const U8 nLenMantissaOrShift = pSC->m_pNextSC->WMISCP.nLenMantissaOrShift;
                const I16 * pSrc = (I16 *)pSrc0;
            
                for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cStride)
                    pA[((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf]] = (((PixelI)pSrc[iAlphaPos] >> nLenMantissaOrShift) << cShift);
            }
            else if(bdExt == BD_16F){
                const size_t cStride = (pSC->WMII.cBitsPerUnit >> 3) / sizeof(U16);
                const I16 * pSrc = (I16 *)pSrc0;

                for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cStride)
                    pA[((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf]] = forwardHalf (pSrc[iAlphaPos]) << cShift;
            }
            else if(bdExt == BD_32S){
                const size_t cStride = (pSC->WMII.cBitsPerUnit >> 3) / sizeof(I32);
                const U8 nLenMantissaOrShift = pSC->m_pNextSC->WMISCP.nLenMantissaOrShift;
                const I32 * pSrc = (I32 *)pSrc0;
            
                for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cStride)
                    pA[((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf]] = (((PixelI)pSrc[iAlphaPos] >> nLenMantissaOrShift) << cShift);
            }
            else if(bdExt == BD_32F){
                const size_t cStride = (pSC->WMII.cBitsPerUnit >> 3) / sizeof(float);
                const U8 nLen = pSC->m_pNextSC->WMISCP.nLenMantissaOrShift;
                const I8 nExpBias = pSC->m_pNextSC->WMISCP.nExpBias;
                const float * pSrc = (float *)pSrc0;
            
                for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cStride)
                    pA[((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf]] = float2pixel (pSrc[iAlphaPos], nExpBias, nLen) << cShift;
            }
            else // not supported
                return ICERR_ERROR;

            if(iRow + 1 < cRow) // vertical padding!
                pSrc0 += pSC->WMIBI.cbStride;

            for(iColumn = cColumn; iColumn < pSC->cmbWidth * 16; iColumn ++) // horizontal padding
                pA[((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf]] =  pA[(((cColumn - 1) >> 4) << 8) + idxCC[iRow][(cColumn - 1) & 0xf]];
        }
    }
    
    return ICERR_OK;
}

// input one MB row of image data from input buffer
Int inputMBRow(CWMImageStrCodec* pSC)
{
    const size_t cShift = (pSC->m_param.bScaledArith ? (SHIFTZERO + QPFRACBITS) : 0);
    const BITDEPTH_BITS bdExt = pSC->WMII.bdBitDepth;
    COLORFORMAT cfExt = pSC->WMII.cfColorFormat;
    const COLORFORMAT cfInt = pSC->m_param.cfColorFormat;
    const size_t cPixelStride = (pSC->WMII.cBitsPerUnit >> 3);
    const size_t iRowStride = 
		(cfExt == YUV_420 || (pSC->WMISCP.bYUVData && pSC->m_param.cfColorFormat==YUV_420)) ? 2 : 1;
    const size_t cRow = pSC->WMIBI.cLine;
    const size_t cColumn = pSC->WMII.cWidth;
	const size_t iB = (pSC->WMII.bRGB ? 2 : 0);
    const size_t iR = 2 - iB;
    const U8 * pSrc0 = (U8 *)pSC->WMIBI.pv;
    const U8 nLen = pSC->WMISCP.nLenMantissaOrShift;
    const I8 nExpBias = pSC->WMISCP.nExpBias;

    PixelI *pY = pSC->p1MBbuffer[0], *pU = pSC->p1MBbuffer[1], *pV = pSC->p1MBbuffer[2];
    size_t iRow, iColumn, iPos;

    // guard input buffer
    if(checkImageBuffer(pSC, cColumn, cRow) != ICERR_OK)
        return ICERR_ERROR;

    if(pSC->m_bUVResolutionChange)  // will do downsampling somewhere else!
        pU = pSC->pResU, pV = pSC->pResV;
    else if(cfInt == Y_ONLY) // xxx to Y_ONLY transcoding!
        pU = pV = pY; // write pY AFTER pU and pV so Y will overwrite U&V

    for(iRow = 0; iRow < 16; iRow += iRowStride){
        if (pSC->WMISCP.bYUVData){
            I32 * pSrc = (I32 *)pSrc0 + pSC->WMII.cLeadingPadding;

            switch(pSC->m_param.cfColorFormat){
            case Y_ONLY:
            case YUV_444:
            case NCOMPONENT:
                {
                    const size_t cChannel = pSC->m_param.cNumChannels;
                    PixelI * pChannel[16];
                    size_t iChannel;

                    assert(cChannel <= 16);
                    for(iChannel = 0; iChannel < cChannel; iChannel ++)
                        pChannel[iChannel & 15] = pSC->p1MBbuffer[iChannel & 15];
                    if(pSC->m_bUVResolutionChange)
                        pChannel[1] = pSC->pResU, pChannel[2] = pSC->pResV;

                    for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cChannel){
                        iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
                        for(iChannel = 0; iChannel < cChannel; iChannel ++)
                            pChannel[iChannel & 15][iPos] = (PixelI)pSrc[iChannel & 15];
                    }
                }
                break;

            case YUV_422:
                for(iColumn = 0; iColumn < cColumn; iColumn += 2, pSrc += 4){
                    if(cfInt != Y_ONLY){
                        iPos = ((iColumn >> 4) << 7) + idxCC[iRow][(iColumn >> 1) & 7];
                        pU[iPos] = (PixelI)pSrc[0];
                        pV[iPos] = (PixelI)pSrc[2];
                    }

                    pY[((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 15]] = (PixelI)pSrc[1];
                    pY[(((iColumn + 1) >> 4) << 8) + idxCC[iRow][(iColumn + 1) & 15]] = (PixelI)pSrc[3];
                }
                break;

            case YUV_420:
                for(iColumn = 0; iColumn < cColumn; iColumn += 2, pSrc += 6){
                    if(cfInt != Y_ONLY){
                        iPos = ((iColumn >> 4) << 6) + idxCC_420[iRow >> 1][(iColumn >> 1) & 7];
                        pU[iPos] = (PixelI)pSrc[4];
                        pV[iPos] = (PixelI)pSrc[5];
                    }

                    pY[((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 15]] = (PixelI)pSrc[0];
                    pY[(((iColumn + 1) >> 4) << 8) + idxCC[iRow][(iColumn + 1) & 15]] = (PixelI)pSrc[1];
                    pY[((iColumn >> 4) << 8) + idxCC[iRow + 1][iColumn & 15]] = (PixelI)pSrc[2];
                    pY[(((iColumn + 1) >> 4) << 8) + idxCC[iRow + 1][(iColumn + 1) & 15]] = (PixelI)pSrc[3];
                }
                break;

            default:
                assert(0);
                break;
            }
        }
        else if(bdExt == BD_8){
            const U8 * pSrc = pSrc0 + pSC->WMII.cLeadingPadding;
            const PixelI iOffset = (128 << cShift);

            switch(cfExt){
                case CF_RGB:
                    assert (pSC->m_bSecondary == FALSE);
					for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cPixelStride){
						PixelI r = ((PixelI)pSrc[iR]) << cShift, g = ((PixelI)pSrc[1]) << cShift, b = ((PixelI)pSrc[iB]) << cShift;   

						_CC(r, g, b); // color conversion
				   
						iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
						pU[iPos] = -r, pV[iPos] = b, pY[iPos] = g - iOffset;
					}
                    break;

                case Y_ONLY:
                case YUV_444:
                case NCOMPONENT:
                {
                    const size_t cChannel = pSC->m_param.cNumChannels;
                    PixelI * pChannel[16];
                    size_t iChannel;

                    assert(cChannel <= 16);
                    for(iChannel = 0; iChannel < cChannel; iChannel ++)
                        pChannel[iChannel & 15] = pSC->p1MBbuffer[iChannel & 15];
                    if(pSC->m_bUVResolutionChange)
                        pChannel[1] = pSC->pResU, pChannel[2] = pSC->pResV;

                    for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cPixelStride){
                        iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
                        for(iChannel = 0; iChannel < cChannel; iChannel ++)
                            pChannel[iChannel & 15][iPos] = (((PixelI)pSrc[iChannel & 15]) << cShift) - iOffset;
                    }
                    break;
                }

                case CF_RGBE:
                    for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cPixelStride){
                        PixelI iExp = (PixelI)pSrc[3];
                        PixelI r = forwardRGBE (pSrc[0], iExp) << cShift;
                        PixelI g = forwardRGBE (pSrc[1], iExp) << cShift;
                        PixelI b = forwardRGBE (pSrc[2], iExp) << cShift;

                        _CC(r, g, b);

                        iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
                        pU[iPos] = -r, pV[iPos] = b, pY[iPos] = g;
                    }
                    break;

                case CMYK:
                {
                    PixelI * pK = (cfInt == CMYK ? pSC->p1MBbuffer[3] : pY); // CMYK -> YUV_xxx transcoding!

                    for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cPixelStride){
                        PixelI c = ((PixelI)pSrc[0]) << cShift;
                        PixelI m = ((PixelI)pSrc[1]) << cShift;
                        PixelI y = ((PixelI)pSrc[2]) << cShift;
                        PixelI k = ((PixelI)pSrc[3]) << cShift;

                        _CC_CMYK(c, m, y, k);
                        
                        iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
                        pU[iPos] = c, pV[iPos] = -y, pK[iPos] = k, pY[iPos] = iOffset - m;
                    }
                    break;
                }

                case YUV_422:
                    for(iColumn = 0; iColumn < cColumn; iColumn += 2, pSrc += cPixelStride){
                        if(cfInt != Y_ONLY){
                            iPos = ((iColumn >> 4) << 7) + idxCC[iRow][(iColumn >> 1) & 7];
                            pU[iPos] = (((PixelI)pSrc[0]) << cShift) - iOffset;
                            pV[iPos] = (((PixelI)pSrc[2]) << cShift) - iOffset;
                        }

                        pY[((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 15]] = (((PixelI)pSrc[1]) << cShift) - iOffset;
                        pY[(((iColumn + 1) >> 4) << 8) + idxCC[iRow][(iColumn + 1) & 15]] = (((PixelI)pSrc[3]) << cShift) - iOffset;
                    }
                    break;

                case YUV_420:
                    for(iColumn = 0; iColumn < cColumn; iColumn += 2, pSrc += cPixelStride){
                        if(cfInt != Y_ONLY){
                            iPos = ((iColumn >> 4) << 6) + idxCC_420[iRow >> 1][(iColumn >> 1) & 7];
                            pU[iPos] = (((PixelI)pSrc[4]) << cShift) - iOffset;
                            pV[iPos] = (((PixelI)pSrc[5]) << cShift) - iOffset;
                        }

                        pY[((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 15]] = (((PixelI)pSrc[0]) << cShift) - iOffset;
                        pY[(((iColumn + 1) >> 4) << 8) + idxCC[iRow][(iColumn + 1) & 15]] = (((PixelI)pSrc[1]) << cShift) - iOffset;
                        pY[((iColumn >> 4) << 8) + idxCC[iRow + 1][iColumn & 15]] = (((PixelI)pSrc[2]) << cShift) - iOffset;
                        pY[(((iColumn + 1) >> 4) << 8) + idxCC[iRow + 1][(iColumn + 1) & 15]] = (((PixelI)pSrc[3]) << cShift) - iOffset;
                    }
                    break;

                default:
                    assert(0);
                    break;
            }
        }
        else if(bdExt == BD_16){
            const U16 * pSrc = (U16 *)pSrc0 + pSC->WMII.cLeadingPadding;
            const size_t cStride = cPixelStride / sizeof(U16);
            const PixelI iOffset = ((1 << 15) >> nLen) << cShift;

            switch(cfExt){
                case CF_RGB:
                    for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cStride){
                        PixelI r = ((PixelI)pSrc[0] >> nLen) << cShift, g = ((PixelI)pSrc[1] >> nLen) << cShift, b = ((PixelI)pSrc[2] >> nLen) << cShift;
                        
                        _CC(r, g, b); // color conversion

                        iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
                        pU[iPos] = -r, pV[iPos] = b, pY[iPos] = g - iOffset;
                    }
                    break;

                case Y_ONLY:
                case YUV_444:
                case NCOMPONENT:
                {
                    const size_t cChannel = pSC->WMISCP.cChannel;
                    size_t iChannel;

                    for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cStride){
                        iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
                        for(iChannel = 0; iChannel < cChannel; iChannel ++)
                            pSC->p1MBbuffer[iChannel][iPos] = (((PixelI)pSrc[iChannel] >> nLen) << cShift) - iOffset;
                    }
                    break;
                }

                case CMYK:
                {
                    PixelI * pK = (cfInt == CMYK ? pSC->p1MBbuffer[3] : pY); // CMYK -> YUV_xxx transcoding!

                    for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cStride){
                        PixelI c = ((PixelI)pSrc[0] >> nLen) << cShift;
                        PixelI m = ((PixelI)pSrc[1] >> nLen) << cShift;
                        PixelI y = ((PixelI)pSrc[2] >> nLen) << cShift;
                        PixelI k = ((PixelI)pSrc[3] >> nLen) << cShift;

                        _CC_CMYK(c, m, y, k);
                        
                        iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
                        pU[iPos] = c, pV[iPos] = -y, pK[iPos] = k, pY[iPos] = iOffset - m;
                    }
                    break;
                }

                case YUV_422:
                    for(iColumn = 0; iColumn < cColumn; iColumn += 2, pSrc += cStride){
                        if(cfInt != Y_ONLY){
                            iPos = ((iColumn >> 4) << 7) + idxCC[iRow][(iColumn >> 1) & 7];
                            pU[iPos] = (((PixelI)pSrc[0]) << cShift) - iOffset;
                            pV[iPos] = (((PixelI)pSrc[2]) << cShift) - iOffset;
                        }

                        pY[((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 15]] = (((PixelI)pSrc[1]) << cShift) - iOffset;
                        pY[(((iColumn + 1) >> 4) << 8) + idxCC[iRow][(iColumn + 1) & 15]] = (((PixelI)pSrc[3]) << cShift) - iOffset;
                    }
                    break;

                case YUV_420:
                    for(iColumn = 0; iColumn < cColumn; iColumn += 2, pSrc += cStride){
                        if(cfInt != Y_ONLY){
                            iPos = ((iColumn >> 4) << 6) + idxCC_420[iRow >> 1][(iColumn >> 1) & 7];
                            pU[iPos] = (((PixelI)pSrc[4]) << cShift) - iOffset;
                            pV[iPos] = (((PixelI)pSrc[5]) << cShift) - iOffset;
                        }

                        pY[((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 15]] = (((PixelI)pSrc[0]) << cShift) - iOffset;
                        pY[(((iColumn + 1) >> 4) << 8) + idxCC[iRow][(iColumn + 1) & 15]] = (((PixelI)pSrc[1]) << cShift) - iOffset;
                        pY[((iColumn >> 4) << 8) + idxCC[iRow + 1][iColumn & 15]] = (((PixelI)pSrc[2]) << cShift) - iOffset;
                        pY[(((iColumn + 1) >> 4) << 8) + idxCC[iRow + 1][(iColumn + 1) & 15]] = (((PixelI)pSrc[3]) << cShift) - iOffset;
                    }
                    break;

                default:
                    assert(0);
                    break;
            }
        }
        else if(bdExt == BD_16S){
            const I16 * pSrc = (I16 *)pSrc0 + pSC->WMII.cLeadingPadding;
            const size_t cStride = cPixelStride / sizeof(I16);

            switch(cfExt){
                case CF_RGB:
                    for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cStride){
                        PixelI r = ((PixelI)pSrc[0] >> nLen) << cShift, g = ((PixelI)pSrc[1] >> nLen) << cShift, b = ((PixelI)pSrc[2] >> nLen) << cShift;
                        
                        _CC(r, g, b); // color conversion

                        iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
                        pU[iPos] = -r, pV[iPos] = b, pY[iPos] = g;
                    }
                    break;

                case Y_ONLY:
                case YUV_444:
                case NCOMPONENT:
					{
						const size_t cChannel = pSC->WMISCP.cChannel;
						size_t iChannel;
	
						for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cStride){
							iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
							for(iChannel = 0; iChannel < cChannel; iChannel ++)
								pSC->p1MBbuffer[iChannel][iPos] = (((PixelI)pSrc[iChannel] >> nLen) << cShift);
						}
					}
				    break;

                case CMYK:
					{
						PixelI * pK = (cfInt == CMYK ? pSC->p1MBbuffer[3] : pY); // CMYK -> YUV_xxx transcoding!
	
						for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cStride){
							PixelI c = ((PixelI)pSrc[0] >> nLen) << cShift;
							PixelI m = ((PixelI)pSrc[1] >> nLen) << cShift;
							PixelI y = ((PixelI)pSrc[2] >> nLen) << cShift;
							PixelI k = ((PixelI)pSrc[3] >> nLen) << cShift;
	
							_CC_CMYK(c, m, y, k);
							
							iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
							pU[iPos] = c, pV[iPos] = -y, pK[iPos] = k, pY[iPos] = -m;
						}
					}
					break;

                default:
                    assert(0);
                    break;
            }
        }
        else if(bdExt == BD_16F){
            const I16 * pSrc = (I16 *)pSrc0 + pSC->WMII.cLeadingPadding;
            const size_t cStride = cPixelStride / sizeof(U16);

            switch(cfExt){
                case CF_RGB:
                    for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cStride){
                        PixelI r = forwardHalf (pSrc[0]) << cShift;
                        PixelI g = forwardHalf (pSrc[1]) << cShift;
                        PixelI b = forwardHalf (pSrc[2]) << cShift;
                        
                        _CC(r, g, b); // color conversion

                        iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
                        pU[iPos] = -r, pV[iPos] = b, pY[iPos] = g;
                    }
                    break;

                case Y_ONLY:
                case YUV_444:
                case NCOMPONENT:
					{
						const size_t cChannel = pSC->WMISCP.cChannel; // check xxx => Y_ONLY transcoding!
						size_t iChannel;
	
						for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cStride){
							iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
							for(iChannel = 0; iChannel < cChannel; iChannel ++)
								pSC->p1MBbuffer[iChannel][iPos] = forwardHalf (pSrc[iChannel]) << cShift;
						}
					}
					break;

                default:
                    assert(0);
                    break;
            }
        }
        else if(bdExt == BD_32){
            const U32 * pSrc = (U32 *)pSrc0 + pSC->WMII.cLeadingPadding;
            const size_t cStride = cPixelStride / sizeof(U32);
            const PixelI iOffset = ((1 << 31) >> nLen) << cShift;

            switch(cfExt){
                case CF_RGB:
                    for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cStride){
                        PixelI r = (pSrc[0] >> nLen) << cShift, g = (pSrc[1] >> nLen) << cShift, b = (pSrc[2] >> nLen) << cShift;
                        
                        _CC(r, g, b); // color conversion

                        iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
                        pU[iPos] = -r, pV[iPos] = b, pY[iPos] = g - iOffset;
                    }
                    break;

                case Y_ONLY:
                case YUV_444:
                case NCOMPONENT:
                {
                    const size_t cChannel = pSC->WMISCP.cChannel;
                    size_t iChannel;

                    for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cStride){
                        iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
                        for(iChannel = 0; iChannel < cChannel; iChannel ++)
                            pSC->p1MBbuffer[iChannel][iPos] = (pSrc[iChannel] >> nLen) << cShift;
                    }
                    break;
                }

                default:
                    assert(0);
                    break;
            }
        }
        else if(bdExt == BD_32S){
            const I32 * pSrc = (I32 *)pSrc0 + pSC->WMII.cLeadingPadding;
            const size_t cStride = cPixelStride / sizeof(I32);

            switch(cfExt){
                case CF_RGB:
                    for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cStride){
                        PixelI r = (pSrc[0] >> nLen)<< cShift, g = (pSrc[1] >> nLen)<< cShift, b = (pSrc[2] >> nLen)<< cShift;
                        
                        _CC(r, g, b); // color conversion

                        iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
                        pU[iPos] = -r, pV[iPos] = b, pY[iPos] = g;
                    }
                    break;

                case Y_ONLY:
                case YUV_444:
                case NCOMPONENT:
					{
						const size_t cChannel = pSC->WMISCP.cChannel; // check xxx => Y_ONLY transcoding!
						size_t iChannel;
	
						for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cStride){
							iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
							for(iChannel = 0; iChannel < cChannel; iChannel ++)
								pSC->p1MBbuffer[iChannel][iPos] = (pSrc[iChannel] >> nLen) << cShift;
						}
					}
					break;

                default:
                    assert(0);
                    break;
            }
        }
        else if(bdExt == BD_32F){
            const float * pSrc = (float *)pSrc0 + pSC->WMII.cLeadingPadding;
            const size_t cStride = cPixelStride / sizeof(float);

            switch(cfExt){
                case CF_RGB:
                    for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cStride){
                        PixelI r = float2pixel (pSrc[0], nExpBias, nLen) << cShift;
                        PixelI g = float2pixel (pSrc[1], nExpBias, nLen) << cShift;
                        PixelI b = float2pixel (pSrc[2], nExpBias, nLen) << cShift;

                        _CC(r, g, b); // color conversion

                        iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
                        pU[iPos] = -r, pV[iPos] = b, pY[iPos] = g;
                    }
                    break;

                case Y_ONLY:
                case YUV_444:
                case NCOMPONENT:
					{
						const size_t cChannel = pSC->WMISCP.cChannel;
						size_t iChannel;
	
						for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cStride){
							iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
							for(iChannel = 0; iChannel < cChannel; iChannel ++)
								pSC->p1MBbuffer[iChannel][iPos] = float2pixel (pSrc[iChannel], nExpBias, nLen) << cShift;
						}
					}
					break;
                default:
                    assert(0);
                    break;
            }
        }
        else if(bdExt == BD_5){ // RGB 555, work for both big endian and small endian!
            const U8 * pSrc = pSrc0;
            const PixelI iOffset = (16 << cShift);

            assert(cfExt == CF_RGB);
            
            for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cPixelStride){
                PixelI r = (PixelI)pSrc[0], g = (PixelI)pSrc[1], b = ((g >> 2) & 0x1F) << cShift;

                g = ((r >> 5) + ((g & 3) << 3)) << cShift, r = (r & 0x1F) << cShift;

                _CC(r, g, b); // color conversion
                
                iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
                pU[iPos] = -r, pV[iPos] = b, pY[iPos] = g - iOffset;
            }
        }
        else if(bdExt == BD_565){ // RGB 555, work for both big endian and small endian!
            const U8 * pSrc = pSrc0;
            const PixelI iOffset = (32 << cShift);

            assert(cfExt == CF_RGB);
            
            for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cPixelStride){
                PixelI r = (PixelI)pSrc[0], g = (PixelI)pSrc[1], b = (g >> 3) << (cShift + 1);

                g = ((r >> 5) + ((g & 7) << 3)) << cShift, r = (r & 0x1F) << (cShift + 1);

                _CC(r, g, b); // color conversion
                
                iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
                pU[iPos] = -r, pV[iPos] = b, pY[iPos] = g - iOffset;
            }
        }
        else if(bdExt == BD_10){ //RGB 101010, work for both big endian and small endian!
            const U8 * pSrc = pSrc0;
            const PixelI iOffset = (512 << cShift);

            assert(cfExt == CF_RGB);
            
            for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cPixelStride){
                PixelI r = (PixelI)pSrc[0], g = (PixelI)pSrc[1], b = (PixelI)pSrc[2];

                r = (r + ((g & 3) << 8)) << cShift, g = ((g >> 2) + ((b & 0xF) << 6)) << cShift;
                b = ((b >> 4) + (((PixelI)pSrc[3] & 0x3F) << 4)) << cShift;

                _CC(r, g, b); // color conversion
                
                iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
                pU[iPos] = -r, pV[iPos] = b, pY[iPos] = g - iOffset;
            }
        }
        else if(bdExt == BD_1){
            assert(cfExt == Y_ONLY);
            for(iColumn = 0; iColumn < cColumn; iColumn ++) {
                pY[((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf]] = ((pSC->WMISCP.bBlackWhite + (pSrc0[iColumn >> 3] >> (7 - (iColumn & 7)))) & 1) << cShift;
            }
        }

        if(iRow + iRowStride < cRow) // centralized vertical padding!
            pSrc0 += pSC->WMIBI.cbStride;
    }

    padHorizontally(pSC); // centralized horizontal padding

    // centralized down-sampling
    if(pSC->m_bUVResolutionChange)
        downsampleUV(pSC);

    // centralized alpha channel handdling
    if (pSC->WMISCP.uAlphaMode == 3)
        if(inputMBRowAlpha(pSC) != ICERR_OK)
            return ICERR_ERROR;

    return ICERR_OK;
}


