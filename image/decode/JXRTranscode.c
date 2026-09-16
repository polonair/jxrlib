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

#include "windowsmediaphoto.h"
#include "strcodec.h"
#include "decode.h"
#include "JxrTranscodeTileQuantizerState.h"
#include "JxrTranscodeOrientationState.h"
#include "JxrTranscodeSession.h"
#include "JxrTranscodeSessionFactory.h"
#include "JxrTranscodeDecoderInitializer.h"
#include "JxrTranscodeEncoderInitializer.h"
#include "JxrTranscodeRoiInitializer.h"
#include "JxrTranscodeFrameBufferAllocator.h"
#include "JxrTranscodeAlphaPlaneInitializer.h"
#include "JxrTranscodeDecoderRuntimeInitializer.h"
#include "JxrTranscodeEncoderOutputInitializer.h"
#include "JxrTranscodeMacroblockProcessingPipeline.h"
#include "JxrTranscodeTileExtractionDecision.h"

Bool isTileExtraction(CWMImageStrCodec * pSC, CWMTranscodingParam * pParam)
{
    JxrTranscodeTileExtractionDecision decision = {0};

    decision.tileColumns = pSC->WMISCP.uiTileX;
    decision.tileColumnCount = pSC->WMISCP.cNumOfSliceMinus1V + 1;
    decision.macroblockWidth = (U32)pSC->cmbWidth;
    decision.tileRows = pSC->WMISCP.uiTileY;
    decision.tileRowCount = pSC->WMISCP.cNumOfSliceMinus1H + 1;
    decision.macroblockHeight = (U32)pSC->cmbHeight;
    decision.roiLeftPixels = pParam->cLeftX;
    decision.roiTopPixels = pParam->cTopY;
    decision.roiWidthPixels = pParam->cWidth;
    decision.roiHeightPixels = pParam->cHeight;
    decision.extraLeftPixels = pSC->m_param.cExtraPixelsLeft;
    decision.extraTopPixels = pSC->m_param.cExtraPixelsTop;
    decision.sourceOverlap = pSC->WMISCP.olOverlap;
    decision.ignoreOverlap = pParam->bIgnoreOverlap;
    decision.hasTransform = pParam->oOrientation != O_NONE;
    decision.sourceLayout = pSC->WMISCP.bfBitstreamFormat;
    decision.targetLayout = pParam->bfBitstreamFormat;
    decision.sourceSubband = pSC->WMISCP.sbSubband;
    decision.targetSubband = pParam->sbSubband;
    {
        Bool canUseFastPath = JxrTranscodeTileExtractionDecisionCanUseFastPath(&decision);
        pParam->bIgnoreOverlap = decision.ignoreOverlap;
        return canUseFastPath;
    }
}
Int WMPhotoTranscode(struct WMPStream * pStreamIn, struct WMPStream * pStreamOut, CWMTranscodingParam * pParam)
{
    PixelI * pMBBuf, MBBufAlpha[256]; // shared buffer, decoder <=> encoder bridge
    PixelI * pFrameBuf = NULL, * pFrameBufAlpha = NULL;
    CWMIMBInfo * pMBInfo = NULL, * pMBInfoAlpha = NULL;
    CWMImageStrCodec * pSCDec, * pSCEnc;
    CWMDecoderParameters aDecoderParam = {0};
    JxrTranscodeDecoderInitializationResult decoderInitialization;
    JxrTranscodeEncoderInitializationResult encoderInitialization;
    JxrTranscodeRoiInitializationResult roiInitialization;
    JxrTranscodeFrameBufferAllocation frameBuffers;
    JxrTranscodeAlphaPlaneInitializationResult alphaInitialization;
    JxrTranscodeDecoderRuntimeState decoderRuntime;
    JxrTranscodeEncoderOutputInitializationResult encoderOutputInitialization;
    U8 * pIOHeaderDec, * pIOHeaderEnc;
    JxrTranscodeTileQuantizerState * pTileQPInfo = NULL;
    ORIENTATION oO = pParam->oOrientation;
    JxrTranscodeOrientationState orientation;
    size_t iAlphaPos = 0;
    size_t cUnit;
    size_t mbLeft, mbRight, mbTop, mbBottom, mbWidth, mbHeight;

    if(pStreamIn == NULL || pStreamOut == NULL || pParam == NULL)
        return ICERR_ERROR;

    // initialize decoder
    if(JxrTranscodeSessionFactoryCreateCodec(pStreamIn, &pSCDec) != ICERR_OK)
        return ICERR_ERROR;
    if(JxrTranscodeDecoderInitializerInitialize(pSCDec, pParam, &aDecoderParam,
        &decoderInitialization) != ICERR_OK)
        return ICERR_ERROR;
    oO = decoderInitialization.orientationValue;
    orientation = decoderInitialization.orientation;

    pParam->bIgnoreOverlap = isTileExtraction(pSCDec, pParam);

    cUnit = decoderInitialization.coefficientUnit;
    if(JxrTranscodeDecoderRuntimeInitializerAllocateMacroblockBuffer(pSCDec,
        cUnit, &decoderRuntime) != ICERR_OK)
        return ICERR_ERROR;
    pMBBuf = decoderRuntime.macroblockBuffer;

    if(JxrTranscodeAlphaPlaneInitializerInitializeDecoder(pSCDec, pParam,
        MBBufAlpha, &alphaInitialization) != ICERR_OK)
        return ICERR_ERROR;
    iAlphaPos = alphaInitialization.channelIndex;

    if(JxrTranscodeDecoderRuntimeInitializerInitializePrimaryInput(pSCDec,
        &decoderRuntime) != ICERR_OK)
        return ICERR_ERROR;
    pIOHeaderDec = decoderRuntime.ioHeaderAllocation;

    if(JxrTranscodeAlphaPlaneInitializerFinalizeDecoder(pSCDec) != ICERR_OK)
        return ICERR_ERROR;

    // initialize encoder
    if(JxrTranscodeEncoderInitializerInitialize(pSCDec, pStreamOut, pParam,
        &encoderInitialization) != ICERR_OK)
        return ICERR_ERROR;
    pSCEnc = encoderInitialization.encoderCodec;
    pIOHeaderEnc = encoderInitialization.ioHeaderAllocation;

    if(JxrTranscodeRoiInitializerInitialize(pSCDec, pSCEnc, pParam,
        &orientation, &roiInitialization) != ICERR_OK)
        return ICERR_ERROR;
    mbLeft = roiInitialization.macroblockLeft;
    mbRight = roiInitialization.macroblockRight;
    mbTop = roiInitialization.macroblockTop;
    mbBottom = roiInitialization.macroblockBottom;
    mbWidth = roiInitialization.macroblockWidth;
    mbHeight = roiInitialization.macroblockHeight;

    if(JxrTranscodeFrameBufferAllocatorAllocate(pSCEnc, pParam, oO, cUnit,
        &frameBuffers) != ICERR_OK)
        return ICERR_ERROR;
    pFrameBuf = frameBuffers.primaryCoefficients;
    pFrameBufAlpha = frameBuffers.alphaCoefficients;
    pMBInfo = frameBuffers.primaryMacroblocks;
    pMBInfoAlpha = frameBuffers.alphaMacroblocks;

    if(JxrTranscodeEncoderOutputInitializerInitialize(pSCDec, pSCEnc, pParam,
        oO, &orientation, &encoderOutputInitialization) != ICERR_OK)
        return ICERR_ERROR;
    pTileQPInfo = encoderOutputInitialization.tileQuantizers;

    if(JxrTranscodeAlphaPlaneInitializerInitializeEncoder(pSCDec, pSCEnc,
        pParam) != ICERR_OK)
        return ICERR_ERROR;

    {
        JxrTranscodeMacroblockProcessingPipeline pipeline = {0};

        pipeline.decoderCodec = pSCDec;
        pipeline.encoderCodec = pSCEnc;
        pipeline.parameters = pParam;
        pipeline.macroblockBuffer = pMBBuf;
        pipeline.alphaMacroblockBuffer = MBBufAlpha;
        pipeline.coefficientUnit = cUnit;
        pipeline.alphaChannelIndex = iAlphaPos;
        pipeline.macroblockLeft = mbLeft;
        pipeline.macroblockRight = mbRight;
        pipeline.macroblockTop = mbTop;
        pipeline.macroblockBottom = mbBottom;
        pipeline.macroblockWidth = mbWidth;
        pipeline.macroblockHeight = mbHeight;
        pipeline.orientationValue = oO;
        pipeline.orientation = &orientation;
        pipeline.tileQuantizers = pTileQPInfo;
        pipeline.tileQuantizerCount = encoderOutputInitialization.tileQuantizerCount;
        pipeline.primaryFrameBuffer = pFrameBuf;
        pipeline.alphaFrameBuffer = pFrameBufAlpha;
        pipeline.primaryFrameMacroblocks = pMBInfo;
        pipeline.alphaFrameMacroblocks = pMBInfoAlpha;
        pipeline.usedFastTileExtraction =
            encoderOutputInitialization.usedFastTileExtraction;
        if(JxrTranscodeMacroblockProcessingPipelineExecute(&pipeline) != ICERR_OK)
            return ICERR_ERROR;
    }

    {
        JxrTranscodeSession session = {0};

        session.macroblockBuffer = pMBBuf;
        session.primaryFrameBuffer = pFrameBuf;
        session.alphaFrameBuffer = pFrameBufAlpha;
        session.primaryFrameMacroblocks = pMBInfo;
        session.alphaFrameMacroblocks = pMBInfoAlpha;
        session.decoderCodec = pSCDec;
        session.encoderCodec = pSCEnc;
        session.decoderIoHeader = pIOHeaderDec;
        session.encoderIoHeader = pIOHeaderEnc;
        session.tileQuantizers = pTileQPInfo;
        session.hasOrientation = oO != O_NONE;
        session.hasAlphaFrame = pParam->uAlphaMode > 0;
        session.decoderHasAlpha = pSCDec->m_param.bAlphaChannel;
        session.usedFastTileExtraction = encoderOutputInitialization.usedFastTileExtraction;
        JxrTranscodeSessionRelease(&session);
    }

    return ICERR_OK;
}
