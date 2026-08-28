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
#include "JxrEncoderChromaResamplingSetup.h"
#include "JxrEncoderTileStateInitializer.h"
#include "JxrEncoderOutputInitializer.h"
#include "JxrEncoderSampleConversion.h"
#include "JxrEncoderColorTransform.h"
#include "JxrEncoderAlphaPlaneInput.h"
#include "JxrEncoderInputPadding.h"
#include "JxrEncoderChromaDownsampler.h"
#include "JxrEncoderInputRowProcessor.h"
#include "JxrEncoderSessionInitializer.h"
#include "JxrEncoderSessionFactory.h"
#include "JxrEncoderResourceRelease.h"
#include "JxrEncoderMemoryLayoutPlan.h"
#include "JxrEncoderBufferRegionLayout.h"
#include "JxrEncoderProcessingPipeline.h"
#include "JxrEncoderMacroblockProcessingPipeline.h"
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

//================================================================
// Color Conversion 
// functions to get image data from input buffer
// this inlcudes necessary color conversion and boundary padding
//================================================================

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
    if (JxrEncoderChromaResamplingSetupInitialize(pSC) != ICERR_OK)
        return ICERR_ERROR;
    if (JxrEncoderTileStateInitializerInitialize(pSC) != ICERR_OK)
        return ICERR_ERROR;
    if (JxrEncoderOutputInitializerInitialize(pSC) != ICERR_OK)
        return ICERR_ERROR;

    return ICERR_OK;
}

static Int StrEncTerm(CTXSTRCODEC ctxSC)
{
    CWMImageStrCodec* codec = (CWMImageStrCodec*)ctxSC;
    JXRTraceDumpCodecState("encoder", codec);
    return JxrEncoderResourceReleaseRelease(codec);
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
    JxrEncoderSessionInitializerInitialize(pSC, pII, pSCP);
}

/*************************************************************************
   Streaming API init
*************************************************************************/
Int ImageStrEncInit(
    CWMImageInfo* pII,
    CWMIStrCodecParam *pSCP,
    CTXSTRCODEC* pctxSC)
{
    return JxrEncoderSessionFactoryCreate(pII, pSCP, pctxSC);
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

    JxrEncoderProcessingPipelinePlan processingPipeline;

#if defined(WMP_OPT_SSE2) || defined(WMP_OPT_CC_ENC) || defined(WMP_OPT_TRFM_ENC)
    JxrEncoderProcessingPipelinePlanInitialize(&processingPipeline, TRUE);
#else
    JxrEncoderProcessingPipelinePlanInitialize(&processingPipeline, FALSE);
#endif

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


    if (processingPipeline.usesLegacyLoadCallback) {
        if (pSC->Load(pSC) != ICERR_OK)
            return ICERR_ERROR;
    }
    else if (JxrEncoderProcessingPipelineLoadInput(pSC) != ICERR_OK)
        return ICERR_ERROR;
    JXRTraceDumpStage("encoder", "centered_samples", pSC, 0, (Int)pSC->cRow, JXRTraceSamples);
    if (JxrEncoderMacroblockProcessingPipelineProcessLoadedRow(pSC) != ICERR_OK)
        return ICERR_ERROR;

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
    JxrEncoderMacroblockProcessingPipelineProcessFinalRow(pSC);

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
// centralized horizontal padding
// centralized alpha channel color conversion, small perf penalty
// input one MB row of image data from input buffer
