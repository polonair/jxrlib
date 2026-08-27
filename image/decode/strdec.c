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
#include "JxrMacroblockRegionState.h"
#include "JxrDecoderTileQuantizerSyntaxReader.h"
#include "JxrImagePlaneQuantizerHeaderReader.h"
#include "JxrImagePlaneDescriptorReader.h"
#include "JxrMainHeaderReader.h"
#include "JxrHeaderStateApplier.h"
#include "JxrHeaderValidation.h"
#include "JxrHeaderStreamReader.h"

#include "JxrHeaderMetadataFinalizer.h"
#include "JxrHeaderDecodePipeline.h"
#include "JxrDecoderInitializationPipeline.h"
#include "JxrDecoderInputInitializer.h"
#include "JxrDecoderSessionPreparation.h"
#include "JxrDecoderRequestValidator.h"
#include "JxrDecoderSessionReleaser.h"
#include "JxrDecoderSessionFactory.h"
#include "JxrDecoderSessionExecutor.h"


#include "JxrDecoderTransformPipeline.h"
#include "JxrDecoderMacroblockProcessingPipeline.h"

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
    CWMIStrCodecParam* pSCP)
{
    return JxrDecoderRequestValidatorReadInfo(pII, pSCP);
}

EXTERN_C Int WMPhotoValidate(
    CWMImageInfo* pII,
    CWMIStrCodecParam* pSCP)
{
    return JxrDecoderRequestValidatorValidateAndNormalize(pII, pSCP);
}
/*************************************************************************
  ImageStrDecInit
*************************************************************************/
Int ImageStrDecInit(
    CWMImageInfo* pII,
    CWMIStrCodecParam *pSCP,
    CTXSTRCODEC* pctxSC)
{
    JxrDecoderSessionPreparation preparation;
    CWMImageStrCodec* primaryCodec = NULL;
    Int result;

    *pctxSC = NULL;
    if (JxrDecoderSessionPreparationPrepare(pII, pSCP, &preparation) != ICERR_OK)
        return ICERR_ERROR;
    result = JxrDecoderSessionFactoryCreate(&preparation, pSCP->fMeasurePerf,
        pII, pSCP, &primaryCodec);
    if (result != ICERR_OK)
        return result;
    *pctxSC = (CTXSTRCODEC)primaryCodec;
    return ICERR_OK;
}

Int ImageStrDecDecode(
    CTXSTRCODEC ctxSC,
    const CWMImageBufferInfo* pBI
#ifdef REENTRANT_MODE
    , size_t* pcDecodedLines
#endif
    )
{
    return JxrDecoderSessionExecutorExecute((CWMImageStrCodec*)ctxSC, pBI
#ifdef REENTRANT_MODE
        , pcDecodedLines
#endif
        );
}
Int ImageStrDecTerm(
    CTXSTRCODEC ctxSC)
{
    return JxrDecoderSessionReleaserRelease((CWMImageStrCodec*)ctxSC);
}

