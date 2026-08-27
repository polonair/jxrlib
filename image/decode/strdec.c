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
#include "decode.h"
#include "JxrMacroblockRegionState.h"
#include "JxrDecoderTileQuantizerSyntaxReader.h"
#include "JxrImagePlaneQuantizerHeaderReader.h"
#include "JxrImagePlaneDescriptorReader.h"
#include "JxrMainHeaderReader.h"
#include "JxrHeaderStateApplier.h"
#include "JxrHeaderValidation.h"
#include "JxrHeaderStreamReader.h"
#include "JxrStreamPositionScope.h"
#include "JxrHeaderMetadataFinalizer.h"
#include "JxrHeaderDecodePipeline.h"
#include "JxrDecoderInitializationPipeline.h"
#include "JxrDecoderInputInitializer.h"
#include "JxrDecoderMemoryLayoutPlan.h"
#include "JxrDecoderSessionPreparation.h"
#include "JxrDecoderPrimaryPlaneFactory.h"
#include "JxrDecoderResourceInitializer.h"
#include "JxrDecoderOutputPipeline.h"
#include "JxrDecoderTransformPipeline.h"
#include "JxrDecoderMacroblockProcessingPipeline.h"
#include "JxrDecoderExecutionPipeline.h"
#include "JxrSecondaryPlaneInitializer.h"
#include "JxrDecoderDcQuantizerHeaderApplier.h"
#include "JxrDecoderLpQuantizerHeaderApplier.h"
#include "JxrDecoderHpQuantizerHeaderApplier.h"
#include "JxrDecoderTileHeaderReader.h"
#include "JxrDecoderCodingContextResetter.h"
#include "JxrDecoderPacketRowReader.h"
#include "JxrInverseColorTransform.h"
#include "JxrSampleClipping.h"
#include "JxrFloatSampleConversion.h"
#include "JxrMonochromeExpansion.h"
#include "JxrDecoderRoiRowRange.h"
#include "JxrVariableLengthWordReader.h"
#include "JxrIndexTableReader.h"
#include "JxrDecoderStreamInitializer.h"
#include "JxrDecoderPacketAttachment.h"
#include "JxrDecoderPacketHeaderReader.h"
#include "JxrPacketHeaderSyntaxReader.h"
#include "strTransform.h"
#include <math.h>
#include "perfTimer.h"

#ifdef MEM_TRACE
#define TRACE_MALLOC    1
#define TRACE_NEW       0
#define TRACE_HEAP      0
#include "memtrace.h"
#endif

#if defined(WMP_OPT_SSE2) || defined(WMP_OPT_CC_DEC) || defined(WMP_OPT_TRFM_DEC)
void StrDecOpt(CWMImageStrCodec* pSC);
#endif // OPT defined



// Inverse color conversion is implemented by JxrInverseColorTransform.


/*************************************************************************
    Read header of image plane
*************************************************************************/
Int ReadImagePlaneHeader(CWMImageInfo* pII, CWMIStrCodecParam *pSCP,
    CCoreParameters *pSC, SimpleBitIO* pSB)
{
    return JxrHeaderDecodePipelineReadImagePlane(pII, pSCP, pSC, pSB) ?
        ICERR_OK : ICERR_ERROR;
}
/*************************************************************************
    Read header of image, and header of FIRST PLANE only
*************************************************************************/
Int ReadWMIHeader(
    CWMImageInfo* pII,
    CWMIStrCodecParam *pSCP,
    CCoreParameters *pSC)
{
    return JxrHeaderDecodePipelineRead(pII, pSCP, pSC) ? ICERR_OK : ICERR_ERROR;
}
//----------------------------------------------------------------
// streaming api init/decode/term
EXTERN_C Int ImageStrDecGetInfo(
    CWMImageInfo* pII,
    CWMIStrCodecParam *pSCP)
{
    JxrStreamPositionScope positionScope;
    CCoreParameters dummyParameters;
    Int readResult;
    Bool restored;

    if (pII == NULL || pSCP == NULL ||
        !JxrStreamPositionScopeCapture(&positionScope, pSCP->pWStream))
        return ICERR_ERROR;
    readResult = ReadWMIHeader(pII, pSCP, &dummyParameters);
    restored = JxrStreamPositionScopeRestore(&positionScope);
    return readResult == ICERR_OK && restored ? ICERR_OK : ICERR_ERROR;
}
EXTERN_C Int WMPhotoValidate(
    CWMImageInfo * pII,
    CWMIStrCodecParam * pSCP)
{
    CWMImageInfo cII;
    CWMIStrCodecParam cSCP = *pSCP;
    size_t cScale = 1;

    if(ImageStrDecGetInfo(&cII, pSCP) != ICERR_OK)
        return ICERR_ERROR;

    // copy over un-overwritable ImageInfo parameters
    pII->bdBitDepth = cII.bdBitDepth;
    pII->cWidth = cII.cWidth;
    pII->cHeight = cII.cHeight;

    if(pII->cWidth == 0 || pII->cHeight == 0)
        return ICERR_ERROR;

    // copy over overwritable CodecParam parameters
    pSCP->bVerbose = cSCP.bVerbose;
    pSCP->cbStream = cSCP.cbStream;
    pSCP->pWStream = cSCP.pWStream;
    if(pSCP->uAlphaMode > 1) // something + alpha
        pSCP->uAlphaMode = cSCP.uAlphaMode; // something + alpha to alpha or something transcoding!

    // validate color transcoding
    if(pSCP->cfColorFormat == NCOMPONENT)
        pII->cfColorFormat = NCOMPONENT;
    if(pSCP->cfColorFormat == CMYK && pII->cfColorFormat != Y_ONLY && pII->cfColorFormat != CF_RGB)
        pII->cfColorFormat = CMYK;
    if(pSCP->cfColorFormat == YUV_422 && pII->cfColorFormat == YUV_420)
        pII->cfColorFormat = YUV_422;
    if(pSCP->cfColorFormat == YUV_444 && (pII->cfColorFormat == YUV_422 || pII->cfColorFormat == YUV_420))
        pII->cfColorFormat = YUV_444;
    if(cII.cfColorFormat == CF_RGB && pII->cfColorFormat != Y_ONLY && 
        pII->cfColorFormat != NCOMPONENT)  // no guarantee that number of channels will be >= 3
        pII->cfColorFormat = cII.cfColorFormat;
    if(cII.cfColorFormat == CF_RGBE)
        pII->cfColorFormat = CF_RGBE;

    // validate thumbnail parameters
    if(pII->cThumbnailWidth == 0 || pII->cThumbnailWidth > pII->cWidth)
        pII->cThumbnailWidth = pII->cWidth;
    if(pII->cThumbnailHeight == 0 || pII->cThumbnailHeight > pII->cHeight)
        pII->cThumbnailHeight = pII->cHeight;
    if((pII->cWidth + pII->cThumbnailWidth - 1) / pII->cThumbnailWidth != (pII->cHeight + pII->cThumbnailHeight - 1) / pII->cThumbnailHeight) {
        while((pII->cWidth + cScale - 1) / cScale > pII->cThumbnailWidth &&
            (pII->cHeight + cScale - 1) / cScale > pII->cThumbnailHeight && (cScale << 1))
            cScale <<= 1;
    }
    else {
        cScale = (pII->cWidth + pII->cThumbnailWidth - 1) / pII->cThumbnailWidth;    
        if (cScale == 0)
            cScale = 1;
    }
    pII->cThumbnailWidth = (pII->cWidth + cScale - 1) / cScale;
    pII->cThumbnailHeight = (pII->cHeight + cScale - 1) / cScale;

    // validate region decode parameters
    if(pII->cROIHeight == 0 || pII->cROIWidth == 0){
        pII->cROILeftX = pII->cROITopY = 0;
        pII->cROIWidth = pII->cThumbnailWidth;
        pII->cROIHeight = pII->cThumbnailHeight;
    }
    if(pII->cROILeftX >= pII->cThumbnailWidth)
        pII->cROILeftX = 0;
    if(pII->cROITopY >= pII->cThumbnailHeight)
        pII->cROITopY = 0;
    if(pII->cROILeftX + pII->cROIWidth > pII->cThumbnailWidth)
        pII->cROIWidth = pII->cThumbnailWidth - pII->cROILeftX;
    if(pII->cROITopY + pII->cROIHeight > pII->cThumbnailHeight)
        pII->cROIHeight = pII->cThumbnailHeight - pII->cROITopY;

    return ICERR_OK;
}

/*************************************************************************
  ImageStrDecInit
*************************************************************************/
Int ImageStrDecInit(
    CWMImageInfo* pII,
    CWMIStrCodecParam *pSCP,
    CTXSTRCODEC* pctxSC)
{
    JxrDecoderMemoryLayoutPlan memoryLayout;
    JxrDecoderSessionPreparation preparation;
    size_t cMacBlock = 0;

    CWMImageStrCodec *pSC = NULL, *pNextSC = NULL;

    Bool bLessThan64Bit = sizeof(void *) < 8;

    *pctxSC = NULL;

    if (JxrDecoderSessionPreparationPrepare(pII, pSCP, &preparation) != ICERR_OK)
        return ICERR_ERROR;

    //================================================
    JxrDecoderMemoryLayoutPlanInitialize(&memoryLayout,
        preparation.templateCodec.WMISCP.bdBitDepth,
        preparation.templateCodec.m_param.cfColorFormat,
        preparation.templateCodec.m_param.cNumChannels,
        preparation.templateCodec.WMII.cWidth, sizeof(*pSC),
        sizeof(CWMDecoderParameters), sizeof(BitIOInfo),
        bLessThan64Bit);
    cMacBlock = memoryLayout.macroblockCount;

    //================================================
    if (!memoryLayout.allocationIsSafe) {
        /** potential overflow - 32 bit pointers insufficient to address cache **/
        return ICERR_ERROR;
    }

    {
        Int primaryResult = JxrDecoderPrimaryPlaneFactoryCreate(&memoryLayout,
            &preparation.templateCodec.m_param, &preparation.templateCodec,
            preparation.usesHardTileBoundaries, pSCP->fMeasurePerf, &pSC);
        if (primaryResult != ICERR_OK)
            return primaryResult;
    }

    // Create and read the alpha plane before the shared decoder initialization pipeline.
    if (pSC->m_param.bAlphaChannel) {
        JxrSecondaryPlaneInitializer secondaryInitializer;
        Int secondaryResult;
        JxrSecondaryPlaneInitializerInit(&secondaryInitializer, pSC,
            &preparation.templateCodec.m_param, &preparation.templateCodec,
            memoryLayout.channelBytes, cMacBlock);
        secondaryResult = JxrSecondaryPlaneInitializerRun(&secondaryInitializer, &pNextSC);
        if (secondaryResult != ICERR_OK)
            return secondaryResult;
    }
    else
        pSC->WMISCP.uAlphaMode = 0;
    //================================================
    {
        JxrDecoderInitializationPipeline initialization;
        JxrDecoderInitializationPipelineInit(&initialization, pSC, pNextSC);
        if (JxrDecoderInitializationPipelineRun(&initialization) != ICERR_OK)
            return ICERR_ERROR;
    }
    //================================================
    *pII = pSC->WMII;
    *pSCP = pSC->WMISCP;
    *pctxSC = (CTXSTRCODEC)pSC;

    if(pSC->WMII.cPostProcStrength){
        initPostProc(pSC->pPostProcInfo, pSC->cmbWidth, pSC->m_param.cNumChannels);
        if (pSC->m_param.bAlphaChannel) 
            initPostProc(pNextSC->pPostProcInfo, pNextSC->cmbWidth, pNextSC->m_param.cNumChannels);
    }

    PERFTIMER_STOP(pSC->m_fMeasurePerf, pSC->m_ptEncDecPerf);

    return ICERR_OK;
}

Int ImageStrDecDecode(
    CTXSTRCODEC ctxSC,
    const CWMImageBufferInfo* pBI
#ifdef REENTRANT_MODE
    , size_t *pcDecodedLines
#endif
    )
{
    CWMImageStrCodec* pSC = (CWMImageStrCodec*)ctxSC;
    JXRTraceDumpCodecState("decoder", pSC);
    JxrDecoderExecutionPreparation preparation;
    if (sizeof(*pSC) != pSC->cbStruct)
    {
        return ICERR_ERROR;
    }

    //================================
    PERFTIMER_START(pSC->m_fMeasurePerf, pSC->m_ptEncDecPerf);

    if (JxrDecoderExecutionPipelinePrepare(pSC, pBI, &preparation) != ICERR_OK)
        return ICERR_ERROR;
    if (JxrDecoderExecutionPipelineRun(pSC, preparation.macroblockRowCount,
        preparation.usesLegacyLoadCallback
#ifdef REENTRANT_MODE
        , pcDecodedLines
#endif
        ) != ICERR_OK)
        return ICERR_ERROR;
#ifndef REENTRANT_MODE
    JxrDecoderOutputPipelineFinalize(pSC, pBI);
#endif // REENTRANT_MODE

    PERFTIMER_STOP(pSC->m_fMeasurePerf, pSC->m_ptEncDecPerf);
    return ICERR_OK;
}

Int ImageStrDecTerm(
    CTXSTRCODEC ctxSC)
{
    CWMImageStrCodec* pSC = (CWMImageStrCodec*)ctxSC;
    JXRTraceDumpCodecState("decoder", pSC);
    if (NULL == pSC)
    {
        return ICERR_OK;
    }
    if (sizeof(*pSC) != pSC->cbStruct)
    {
        return ICERR_ERROR;
    }

    PERFTIMER_START(pSC->m_fMeasurePerf, pSC->m_ptEncDecPerf);

    JxrDecoderResourceInitializerRelease(pSC);
    PERFTIMER_STOP(pSC->m_fMeasurePerf, pSC->m_ptEncDecPerf);
    PERFTIMER_REPORT(pSC->m_fMeasurePerf, pSC);
    PERFTIMER_DELETE(pSC->m_fMeasurePerf, pSC->m_ptEncDecPerf);
    PERFTIMER_DELETE(pSC->m_fMeasurePerf, pSC->m_ptEndToEndPerf);

    JxrDecoderPrimaryPlaneFactoryRelease(pSC);

    return ICERR_OK;
}

