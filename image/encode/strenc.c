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
#include "JxrEncoderSessionReleaser.h"
#include "JxrEncoderSessionEncoder.h"
#include "JxrEncoderRequestValidator.h"
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
U32 setUniformTiling(U32* pTile, U32 cNumTile, U32 cNumMB)
{
    return JxrEncoderRequestValidatorSetUniformTiling(pTile, cNumTile, cNumMB);
}

U32 validateTiling(U32* pTile, U32 cNumTile, U32 cNumMB)
{
    return JxrEncoderRequestValidatorNormalizeTiling(pTile, cNumTile, cNumMB);
}

Int ValidateArgs(CWMImageInfo* pII, CWMIStrCodecParam* pSCP)
{
    return JxrEncoderRequestValidatorValidateAndNormalize(pII, pSCP);
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
    return JxrEncoderSessionEncoderEncodeRow(pSC, pBI);
}
/*************************************************************************
   Streaming API term
*************************************************************************/
Int ImageStrEncTerm(
    CTXSTRCODEC ctxSC)
{
    CWMImageStrCodec* pSC = (CWMImageStrCodec*)ctxSC;
    JXRTraceDumpCodecState("encoder", pSC);
    return JxrEncoderSessionReleaserRelease(pSC);
}

// centralized UV downsampling
// centralized horizontal padding
// centralized alpha channel color conversion, small perf penalty
// input one MB row of image data from input buffer
