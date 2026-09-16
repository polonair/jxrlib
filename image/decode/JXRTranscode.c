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
#include "JxrTranscodeMacroblockTransform.h"
#include "JxrTranscodeMacroblockDecoder.h"
#include "JxrTranscodeTileContextResolver.h"
#include "JxrTranscodeTileQuantizerCapture.h"
#include "JxrTranscodeTileHeaderEmitter.h"
#include "JxrTranscodeDirectMacroblockEncoder.h"
#include "JxrTranscodeOrientedMacroblockBuffer.h"
#include "JxrTranscodeOrientedMacroblockEncoder.h"
#include "JxrTranscodeTileExtractionExecutor.h"
#include "JxrTranscodeSession.h"
#include "JxrTranscodeSessionFactory.h"
#include "JxrTranscodeDecoderInitializer.h"
#include "JxrTranscodeEncoderInitializer.h"
#include "JxrTranscodeRoiInitializer.h"
#include "JxrTranscodeFrameBufferAllocator.h"
#include "JxrTranscodeAlphaPlaneInitializer.h"
#include "JxrTranscodeDecoderRuntimeInitializer.h"
#include "JxrTranscodeTileExtractionDecision.h"

EXTERN_C Int StrEncInit(CWMImageStrCodec *);
EXTERN_C Int  encodeMB(CWMImageStrCodec *, Int, Int);
EXTERN_C Int  writeIndexTableNull(CWMImageStrCodec *);

EXTERN_C Int WriteWMIHeader(CWMImageStrCodec *);
EXTERN_C Int writeIndexTable(CWMImageStrCodec *);
EXTERN_C Int copyTo(struct WMPStream *, struct WMPStream *, size_t);

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

    {
        JxrTranscodeOrientationState sourceOrientation;
        JxrTranscodeOrientationStateInit(&sourceOrientation, pSCEnc->WMII.oOrientation);
    if(orientation.transpose == FALSE && sourceOrientation.transpose == FALSE)

        pSCEnc->WMII.oOrientation ^= oO;
    else if(orientation.transpose && sourceOrientation.transpose){
        pSCEnc->WMII.oOrientation ^= oO;
        pSCEnc->WMII.oOrientation = (pSCEnc->WMII.oOrientation & 1) * 2 + (pSCEnc->WMII.oOrientation >> 1);
    }
    else if(orientation.transpose && sourceOrientation.transpose == FALSE)
        pSCEnc->WMII.oOrientation = oO ^ ((pSCEnc->WMII.oOrientation & 1) * 2 + (pSCEnc->WMII.oOrientation >> 1));
    else
        pSCEnc->WMII.oOrientation ^= ((oO & 1) * 2 + (oO >> 1));
    }
    
//    pSCEnc->WMISCP.nExpBias += 128;

    if(pParam->bIgnoreOverlap == TRUE){
        attachISWrite(pSCEnc->pIOHeader, pSCEnc->WMISCP.pWStream);
        pSCEnc->pTile = pSCDec->pTile;
        if(pSCEnc->WMISCP.cNumOfSliceMinus1H + pSCEnc->WMISCP.cNumOfSliceMinus1V == 0 && pSCEnc->WMISCP.bfBitstreamFormat == SPATIAL)
            pSCEnc->m_param.bIndexTable = FALSE;
        WriteWMIHeader(pSCEnc);
    }
    else{
        pTileQPInfo = (JxrTranscodeTileQuantizerState *)malloc((oO == O_NONE ? 1 : (pSCEnc->WMISCP.cNumOfSliceMinus1H + 1) * (pSCEnc->WMISCP.cNumOfSliceMinus1V + 1)) * sizeof(JxrTranscodeTileQuantizerState));
        if(pTileQPInfo == NULL || ((oO == O_NONE ? 1 : (pSCEnc->WMISCP.cNumOfSliceMinus1H + 1) * (pSCEnc->WMISCP.cNumOfSliceMinus1V + 1)) * sizeof(JxrTranscodeTileQuantizerState) < (oO == O_NONE ? 1 : (pSCEnc->WMISCP.cNumOfSliceMinus1H + 1) * (pSCEnc->WMISCP.cNumOfSliceMinus1V + 1))))
            return ICERR_ERROR;
        
        if(StrEncInit(pSCEnc) != ICERR_OK)
            return ICERR_ERROR;
    }

    if(JxrTranscodeAlphaPlaneInitializerInitializeEncoder(pSCDec, pSCEnc,
        pParam) != ICERR_OK)
        return ICERR_ERROR;

    if(pParam->bIgnoreOverlap == TRUE){
        if(JxrTranscodeTileExtractionExecutorExecute(pSCDec, pSCEnc, mbLeft,
            mbRight, mbTop, mbBottom) != ICERR_OK)
            return ICERR_ERROR;
    }
    else
        writeIndexTableNull(pSCEnc);

    for(pSCDec->cRow = 0; pSCDec->cRow < mbBottom && pParam->bIgnoreOverlap == FALSE; pSCDec->cRow ++){
        for(pSCDec->cColumn = 0; pSCDec->cColumn < pSCDec->cmbWidth; pSCDec->cColumn ++){
            Int cRow = (Int)pSCDec->cRow, cColumn = (Int)pSCDec->cColumn;
            
            memset(pMBBuf, 0, sizeof(PixelI) * cUnit);
            if(pSCDec->m_param.bAlphaChannel){ // alpha channel
                memset(pSCDec->m_pNextSC->p1MBbuffer[0], 0, sizeof(PixelI) * 256);
                pSCDec->m_pNextSC->cRow = pSCDec->cRow;
                pSCDec->m_pNextSC->cColumn = pSCDec->cColumn;
            }

            if(JxrTranscodeMacroblockDecoderDecode(pSCDec, cColumn, cRow) != ICERR_OK)
                return ICERR_ERROR;

            {
                JxrTranscodeTileContextRequest tileContextRequest = {0};
                JxrTranscodeTileContextResult tileContext;

                tileContextRequest.sourceRow = pSCDec->cRow;
                tileContextRequest.sourceColumn = pSCDec->cColumn;
                tileContextRequest.macroblockLeft = mbLeft;
                tileContextRequest.macroblockRight = mbRight;
                tileContextRequest.macroblockTop = mbTop;
                tileContextRequest.macroblockBottom = mbBottom;
                tileContextRequest.macroblockWidth = mbWidth;
                tileContextRequest.macroblockHeight = mbHeight;
                tileContextRequest.tileColumns = pSCEnc->WMISCP.uiTileX;
                tileContextRequest.tileColumnCount = pSCEnc->WMISCP.cNumOfSliceMinus1V + 1;
                tileContextRequest.tileRows = pSCEnc->WMISCP.uiTileY;
                tileContextRequest.tileRowCount = pSCEnc->WMISCP.cNumOfSliceMinus1H + 1;
                tileContextRequest.orientation = &orientation;
                if(JxrTranscodeTileContextResolverResolve(&tileContextRequest, &tileContext) == FALSE)
                    return ICERR_ERROR;
                if(tileContext.isInsideRoi){
                    cRow = tileContext.destinationRow;
                    cColumn = tileContext.destinationColumn;
                    pSCEnc->m_bCtxLeft = tileContext.isTileColumnStart;
                    pSCEnc->m_bCtxTop = tileContext.isTileRowStart;
                    if(pSCEnc->m_bCtxLeft)
                        pSCEnc->cTileColumn = tileContext.tileColumn;
                    if(pSCEnc->m_bCtxTop)
                        pSCEnc->cTileRow = tileContext.tileRow;

                if(pSCEnc->m_bCtxLeft && pSCEnc->m_bCtxTop){ // a new tile, buffer tile DQuant info
                    JxrTranscodeTileQuantizerCaptureRequest quantizerCapture = {0};

                    quantizerCapture.states = pTileQPInfo;
                    quantizerCapture.stateCount = oO == O_NONE ? 1 :
                        (pSCEnc->WMISCP.cNumOfSliceMinus1H + 1) *
                        (pSCEnc->WMISCP.cNumOfSliceMinus1V + 1);
                    quantizerCapture.destinationTileRow = pSCEnc->cTileRow;
                    quantizerCapture.destinationTileColumn = pSCEnc->cTileColumn;
                    quantizerCapture.destinationTileColumnCount =
                        pSCEnc->WMISCP.cNumOfSliceMinus1V + 1;
                    quantizerCapture.storeByDestinationTile = oO != O_NONE;
                    quantizerCapture.primaryTile = pSCDec->pTile + pSCDec->cTileColumn;
                    quantizerCapture.primaryChannelCount = pSCEnc->WMISCP.cChannel;
                    quantizerCapture.subband = pSCEnc->WMISCP.sbSubband;
                    quantizerCapture.hasAlpha = pParam->uAlphaMode > 0;
                    if(quantizerCapture.hasAlpha){
                        quantizerCapture.alphaTile = pSCDec->m_pNextSC->pTile +
                            pSCDec->cTileColumn;
                        quantizerCapture.alphaChannelIndex = iAlphaPos;
                    }
                    if(JxrTranscodeTileQuantizerCaptureCapture(&quantizerCapture) == FALSE)
                        return ICERR_ERROR;
                }

                if(oO == O_NONE){
                    if(JxrTranscodeDirectMacroblockEncoderEncode(pSCDec, pSCEnc,
                        mbLeft, mbTop, cColumn, cRow, pTileQPInfo,
                        pParam->uAlphaMode > 0) != ICERR_OK)
                        return ICERR_ERROR;
                }
                else{
                    JxrTranscodeOrientedMacroblockBufferRequest bufferRequest = {0};

                    bufferRequest.primaryMacroblock = &pSCDec->MBInfo;
                    bufferRequest.primaryCoefficients = pMBBuf;
                    bufferRequest.primaryCoefficientCount = cUnit;
                    bufferRequest.primaryFrameMacroblocks = pMBInfo;
                    bufferRequest.primaryFrameMacroblockCount =
                        pSCEnc->cmbWidth * pSCEnc->cmbHeight;
                    bufferRequest.primaryFrameCoefficients = pFrameBuf;
                    bufferRequest.primaryFrameCoefficientCount =
                        pSCEnc->cmbWidth * pSCEnc->cmbHeight * cUnit;
                    bufferRequest.destinationRow = cRow;
                    bufferRequest.destinationColumn = cColumn;
                    bufferRequest.sourceMacroblockWidth = mbWidth;
                    bufferRequest.sourceMacroblockHeight = mbHeight;
                    bufferRequest.orientation = &orientation;
                    bufferRequest.hasAlpha = pParam->uAlphaMode > 0;
                    if(bufferRequest.hasAlpha){
                        bufferRequest.alphaMacroblock = &pSCDec->m_pNextSC->MBInfo;
                        bufferRequest.alphaCoefficients = MBBufAlpha;
                        bufferRequest.alphaCoefficientCount = 256;
                        bufferRequest.alphaFrameMacroblocks = pMBInfoAlpha;
                        bufferRequest.alphaFrameMacroblockCount =
                            pSCEnc->cmbWidth * pSCEnc->cmbHeight;
                        bufferRequest.alphaFrameCoefficients = pFrameBufAlpha;
                        bufferRequest.alphaFrameCoefficientCount =
                            pSCEnc->cmbWidth * pSCEnc->cmbHeight * 256;
                    }
                    if(JxrTranscodeOrientedMacroblockBufferStore(&bufferRequest) == FALSE)
                        return ICERR_ERROR;
                }
            }
            }
        }

        advanceOneMBRow(pSCDec);

        if(oO == O_NONE)
            advanceOneMBRow(pSCEnc);
    }

    if(oO != O_NONE){
        JxrTranscodeOrientedMacroblockEncoderRequest orientedEncoder = {0};

        orientedEncoder.destinationCodec = pSCEnc;
        orientedEncoder.sourceAlphaCodec = pSCDec;
        orientedEncoder.primaryMacroblocks = pMBInfo;
        orientedEncoder.primaryCoefficients = pFrameBuf;
        orientedEncoder.coefficientUnit = cUnit;
        orientedEncoder.macroblockCount = pSCEnc->cmbWidth * pSCEnc->cmbHeight;
        orientedEncoder.destinationCoefficients = pMBBuf;
        orientedEncoder.orientation = &orientation;
        orientedEncoder.tileQuantizers = pTileQPInfo;
        orientedEncoder.tileQuantizerCount =
            (pSCEnc->WMISCP.cNumOfSliceMinus1H + 1) *
            (pSCEnc->WMISCP.cNumOfSliceMinus1V + 1);
        orientedEncoder.tileColumnCount = pSCEnc->WMISCP.cNumOfSliceMinus1V + 1;
        orientedEncoder.hasAlpha = pParam->uAlphaMode > 0;
        if(orientedEncoder.hasAlpha){
            orientedEncoder.alphaMacroblocks = pMBInfoAlpha;
            orientedEncoder.alphaCoefficients = pFrameBufAlpha;
            orientedEncoder.alphaDestinationCoefficients = MBBufAlpha;
        }
        if(JxrTranscodeOrientedMacroblockEncoderEncode(&orientedEncoder) != ICERR_OK)
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
        session.usedFastTileExtraction = pParam->bIgnoreOverlap;
        JxrTranscodeSessionRelease(&session);
    }

    return ICERR_OK;
}
