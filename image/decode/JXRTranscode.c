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
#include "JxrTranscodeQuantizerWriter.h"
#include "JxrTranscodeTileHeaderWriter.h"
#include "JxrTranscodeOrientationState.h"
#include "JxrTranscodeCoefficientTransform.h"
#include "JxrTranscodeTileExtractionDecision.h"
#include "JxrTranscodeRoiGeometry.h"
#include "JxrTranscodeRoiTileLayout.h"
#include "JxrDecoderCoefficientPredictor.h"
#include "JxrDecoderPacketPipeline.h"

EXTERN_C Void freePredInfo(CWMImageStrCodec *);

EXTERN_C Int ReadWMIHeader(CWMImageInfo *, CWMIStrCodecParam *, CCoreParameters *);
#include "JxrDecoderResourceInitializer.h"
#include "JxrDecoderInputInitializer.h"

EXTERN_C Int DecodeMacroblockDC(CWMImageStrCodec *, CCodingContext *, Int, Int);
EXTERN_C Int DecodeMacroblockLowpass(CWMImageStrCodec *, CCodingContext *, Int, Int);
EXTERN_C Int DecodeMacroblockHighpass(CWMImageStrCodec *, CCodingContext *, Int, Int);
EXTERN_C Void FreeCodingContextDec(CWMImageStrCodec *);

EXTERN_C Int StrEncInit(CWMImageStrCodec *);
EXTERN_C Void StrIOEncTerm(CWMImageStrCodec *);
EXTERN_C Void FreeCodingContextEnc(CWMImageStrCodec *);
EXTERN_C Int  encodeMB(CWMImageStrCodec *, Int, Int);
EXTERN_C Int  writeIndexTableNull(CWMImageStrCodec *);

EXTERN_C Int WriteWMIHeader(CWMImageStrCodec *);
EXTERN_C Int ReadImagePlaneHeader(CWMImageInfo *, CWMIStrCodecParam *, CCoreParameters *, SimpleBitIO *);
EXTERN_C Int WriteImagePlaneHeader(CWMImageStrCodec *);
EXTERN_C Int writeIndexTable(CWMImageStrCodec *);
EXTERN_C Int copyTo(struct WMPStream *, struct WMPStream *, size_t);

static Void JxrTranscodeSwapSize(size_t * left, size_t * right)
{
    size_t temporary = *left;
    *left = *right;
    *right = temporary;
}
Void transcodeTileHeader(CWMImageStrCodec * pSC, JxrTranscodeTileQuantizerState * pTileQPInfo)
{
    if(pSC->m_bCtxLeft && pSC->m_bCtxTop && pSC->m_bSecondary == FALSE){
        CCodingContext * pContext = &pSC->m_pCodingContext[pSC->cTileColumn];
        CWMITile * pTile = pSC->pTile + pSC->cTileColumn;
        CWMImageStrCodec * pSCAlpha = (pSC->m_param.bAlphaChannel ? pSC->m_pNextSC : NULL);
        JxrTranscodeBitSink dcOutput, lowpassOutput, highpassOutput, flexbitsOutput;
        JxrTranscodeTileHeaderState state = {0};
        JxrTranscodeTileHeaderResult result;

        JxrTranscodeBitSinkInitLegacy(&dcOutput, pContext->m_pIODC);
        JxrTranscodeBitSinkInitLegacy(&lowpassOutput, pContext->m_pIOLP);
        JxrTranscodeBitSinkInitLegacy(&highpassOutput, pContext->m_pIOAC);
        JxrTranscodeBitSinkInitLegacy(&flexbitsOutput, pContext->m_pIOFL);
        state.isSpatial = pSC->WMISCP.bfBitstreamFormat == SPATIAL;
        state.subband = pSC->WMISCP.sbSubband;
        state.quantizerMode = pSC->m_param.uQPMode;
        state.hasAlpha = pSCAlpha != NULL;
        state.trimFlexbits = pSC->m_param.bTrimFlexbitsFlag;
        state.trimFlexbitsValue = (U8)pContext->m_iTrimFlexBits;
        state.tileId = (U8)((pSC->cTileRow * (pSC->WMISCP.cNumOfSliceMinus1V + 1) +
            pSC->cTileColumn) & 0x1F);
        state.channelCount = pSC->WMISCP.cChannel;
        state.alphaChannelIndex = pSC->m_param.cNumChannels;
        state.quantizers = pTileQPInfo;
        state.dcOutput = &dcOutput;
        state.lowpassOutput = &lowpassOutput;
        state.highpassOutput = &highpassOutput;
        state.flexbitsOutput = &flexbitsOutput;
        if(JxrTranscodeTileHeaderWriterWrite(&state, &result) == FALSE)
            return;

        pTile->cBitsLP = result.lowpassQuantizerBits;
        pTile->cBitsHP = result.highpassQuantizerBits;
        if(pSCAlpha != NULL){
            pTile = pSCAlpha->pTile + pSC->cTileColumn;
            pTile->cBitsLP = result.lowpassAlphaQuantizerBits;
            pTile->cBitsHP = result.highpassAlphaQuantizerBits;
        }
    }
}
Int getROI(CWMImageInfo* pII, CCoreParameters* pCore,
    CWMIStrCodecParam* pSCP, CWMTranscodingParam* pParam)
{
    JxrTranscodeOrientationState orientation;
    JxrTranscodeRoiGeometryRequest request;
    JxrTranscodeRoiGeometryResult result;
    JxrTranscodeRoiTileLayout tileLayout;
    size_t boundaryIndex;

    JxrTranscodeOrientationStateInit(&orientation, pParam->oOrientation);
    memset(&request, 0, sizeof(request));
    request.imageWidth = pII->cWidth;
    request.imageHeight = pII->cHeight;
    request.extraLeft = pCore->cExtraPixelsLeft;
    request.extraTop = pCore->cExtraPixelsTop;
    request.extraRight = pCore->cExtraPixelsRight;
    request.extraBottom = pCore->cExtraPixelsBottom;
    request.requestedLeft = pParam->cLeftX;
    request.requestedTop = pParam->cTopY;
    request.requestedWidth = pParam->cWidth;
    request.requestedHeight = pParam->cHeight;
    request.overlap = pSCP->olOverlap;
    request.ignoreOverlap = pParam->bIgnoreOverlap;
    if (JxrTranscodeRoiGeometryCalculate(&request, &result) == FALSE ||
        JxrTranscodeRoiTileLayoutInitialize(&tileLayout, pSCP->uiTileX,
            (size_t)pSCP->cNumOfSliceMinus1V + 1, pSCP->uiTileY,
            (size_t)pSCP->cNumOfSliceMinus1H + 1) == FALSE ||
        JxrTranscodeRoiTileLayoutApply(&tileLayout, result.macroblockLeft,
            result.macroblockRight, result.macroblockTop, result.macroblockBottom,
            &orientation) == FALSE)
        return ICERR_ERROR;

    pCore->cExtraPixelsLeft = result.extraLeft;
    pCore->cExtraPixelsTop = result.extraTop;
    pCore->cExtraPixelsRight = result.extraRight;
    pCore->cExtraPixelsBottom = result.extraBottom;
    JxrTranscodeRoiTileLayoutOrientExtraPixels(&pCore->cExtraPixelsLeft,
        &pCore->cExtraPixelsTop, &pCore->cExtraPixelsRight,
        &pCore->cExtraPixelsBottom, &orientation);
    pII->cWidth = result.imageWidth;
    pII->cHeight = result.imageHeight;
    pParam->cLeftX = result.expandedLeft;
    pParam->cTopY = result.expandedTop;
    pParam->cWidth = result.expandedWidth;
    pParam->cHeight = result.expandedHeight;

    pSCP->cNumOfSliceMinus1V = (U32)(tileLayout.columnCount - 1);
    pSCP->cNumOfSliceMinus1H = (U32)(tileLayout.rowCount - 1);
    for (boundaryIndex = 0; boundaryIndex < tileLayout.columnCount; boundaryIndex++)
        pSCP->uiTileX[boundaryIndex] = tileLayout.columnBoundaries[boundaryIndex];
    for (boundaryIndex = 0; boundaryIndex < tileLayout.rowCount; boundaryIndex++)
        pSCP->uiTileY[boundaryIndex] = tileLayout.rowBoundaries[boundaryIndex];
    return ICERR_OK;
}
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
    CWMImageStrCodec * pSCDec, * pSCEnc, * pSC;
    CWMDecoderParameters aDecoderParam = {0};
    U8 * pIOHeaderDec, * pIOHeaderEnc;
    CCodingContext * pContext;
    JxrTranscodeTileQuantizerState * pTileQPInfo = NULL;
    ORIENTATION oO = pParam->oOrientation;
    JxrTranscodeOrientationState orientation;
    size_t iAlphaPos = 0;
    size_t cUnit;
    size_t i, j, mbLeft, mbRight, mbTop, mbBottom, mbWidth, mbHeight;

    if(pStreamIn == NULL || pStreamOut == NULL || pParam == NULL)
        return ICERR_ERROR;

    // initialize decoder
    if((pSCDec = (CWMImageStrCodec *)malloc(sizeof(CWMImageStrCodec))) == NULL)
        return ICERR_ERROR;
    memset(pSCDec, 0, sizeof(CWMImageStrCodec));

    pSCDec->WMISCP.pWStream = pStreamIn;
    if(ReadWMIHeader(&pSCDec->WMII, &pSCDec->WMISCP, &pSCDec->m_param) != ICERR_OK)
        return ICERR_ERROR;

    JxrTranscodeOrientationStateInit(&orientation, oO);
    if(pSCDec->WMISCP.cfColorFormat == YUV_422 && orientation.transpose){
        pParam->oOrientation = oO = O_NONE; // Can not rotate 422 in compressed domain!
        JxrTranscodeOrientationStateInit(&orientation, oO);
    }

    pSCDec->cmbWidth = (pSCDec->WMII.cWidth + pSCDec->m_param.cExtraPixelsLeft + pSCDec->m_param.cExtraPixelsRight + 15) / 16;
    pSCDec->cmbHeight = (pSCDec->WMII.cHeight + pSCDec->m_param.cExtraPixelsTop + pSCDec->m_param.cExtraPixelsBottom + 15) / 16;
    pSCDec->m_param.cNumChannels = pSCDec->WMISCP.cChannel;
    pSCDec->m_Dparam = &aDecoderParam;
    pSCDec->m_Dparam->bSkipFlexbits = (pSCDec->WMISCP.sbSubband == SB_NO_FLEXBITS);
    pSCDec->m_param.bTranscode = TRUE;

    pParam->bIgnoreOverlap = isTileExtraction(pSCDec, pParam);

    cUnit = (pSCDec->m_param.cfColorFormat == YUV_420 ? 384 : (pSCDec->m_param.cfColorFormat == YUV_422 ? 512 : 256 * pSCDec->m_param.cNumChannels));
    if(cUnit > 256 * MAX_CHANNELS)
        return ICERR_ERROR;
    pSCDec->p1MBbuffer[0] = pMBBuf = (PixelI *)malloc(cUnit * sizeof(PixelI));
    if(pMBBuf == NULL)
        return ICERR_ERROR;
    pSCDec->p1MBbuffer[1] = pSCDec->p1MBbuffer[0] + 256;
    for(i = 2; i < pSCDec->m_param.cNumChannels; i ++)
        pSCDec->p1MBbuffer[i] = pSCDec->p1MBbuffer[i - 1] + (pSCDec->m_param.cfColorFormat == YUV_420 ? 64 : (pSCDec->m_param.cfColorFormat == YUV_422 ? 128 : 256));

    if(pSCDec->m_param.bAlphaChannel){ // alpha channel
        SimpleBitIO SB = {0};

        iAlphaPos = pSCDec->m_param.cNumChannels;
        if((pSCDec->m_pNextSC = (CWMImageStrCodec *)malloc(sizeof(CWMImageStrCodec))) == NULL)
            return ICERR_ERROR;
        *pSCDec->m_pNextSC = *pSCDec;
        pSCDec->m_pNextSC->p1MBbuffer[0] = MBBufAlpha;
        pSCDec->m_pNextSC->WMISCP.cfColorFormat = pSCDec->m_pNextSC->WMII.cfColorFormat = pSCDec->m_pNextSC->m_param.cfColorFormat = Y_ONLY;
        pSCDec->m_pNextSC->WMISCP.cChannel  = pSCDec->m_pNextSC->m_param.cNumChannels = 1;
        pSCDec->m_pNextSC->m_bSecondary = TRUE;
        pSCDec->m_pNextSC->m_pNextSC = pSCDec;
 
        // read plane header of second image plane
        if(attach_SB(&SB, pSCDec->WMISCP.pWStream) != ICERR_OK)
            return ICERR_ERROR;
        ReadImagePlaneHeader(&pSCDec->m_pNextSC->WMII, &pSCDec->m_pNextSC->WMISCP, &pSCDec->m_pNextSC->m_param, &SB);
        detach_SB(&SB);

        if(JxrDecoderResourceInitializerInitialize(pSCDec->m_pNextSC) != ICERR_OK)
            return ICERR_ERROR;
    }
    else
        pParam->uAlphaMode = 0;

    pIOHeaderDec = (U8 *)malloc((PACKETLENGTH * 4 - 1) + PACKETLENGTH * 4 + sizeof(BitIOInfo));
    if(pIOHeaderDec == NULL)
        return ICERR_ERROR;
    memset(pIOHeaderDec, 0, (PACKETLENGTH * 4 - 1) + PACKETLENGTH * 4 + sizeof(BitIOInfo));
    pSCDec->pIOHeader = (BitIOInfo *)((U8 *)ALIGNUP(pIOHeaderDec, PACKETLENGTH * 4) + PACKETLENGTH * 2);
    
    if(JxrDecoderInputInitializerInitialize(pSCDec) != ICERR_OK)
        return ICERR_ERROR;

    if(JxrDecoderResourceInitializerInitialize(pSCDec) != ICERR_OK)
        return ICERR_ERROR;

    if(pSCDec->m_param.bAlphaChannel){ // alpha channel
        if(JxrDecoderResourceInitializerInitialize(pSCDec->m_pNextSC) != ICERR_OK)
            return ICERR_ERROR;
    }

    // initialize encoder
    if((pSCEnc = (CWMImageStrCodec *)malloc(sizeof(CWMImageStrCodec))) == NULL)
        return ICERR_ERROR;
    memset(pSCEnc, 0, sizeof(CWMImageStrCodec));

    pSCEnc->WMII = pSCDec->WMII;
    pSCEnc->WMISCP = pSCDec->WMISCP;
    pSCEnc->m_param = pSCDec->m_param;
    pSCEnc->WMISCP.pWStream = pStreamOut;
    pSCEnc->WMISCP.bfBitstreamFormat = pParam->bfBitstreamFormat;
//    pSCEnc->m_param.cfColorFormat = pSCEnc->WMISCP.cfColorFormat = pParam->cfColorFormat;
    pSCEnc->m_param.cfColorFormat = pSCEnc->WMISCP.cfColorFormat;
    pSCEnc->m_param.cNumChannels = (pSCEnc->WMISCP.cfColorFormat == Y_ONLY ? 1 : (pSCEnc->WMISCP.cfColorFormat == YUV_444 ? 3 : pSCEnc->WMISCP.cChannel));
    pSCEnc->m_param.bAlphaChannel = (pParam->uAlphaMode > 0);
    pSCEnc->m_param.bTranscode = TRUE;
    if(pParam->sbSubband >= SB_MAX)
        pParam->sbSubband = SB_ALL;
    if(pParam->sbSubband > pSCEnc->WMISCP.sbSubband)
        pSCEnc->WMISCP.sbSubband = pParam->sbSubband;
    pSCEnc->m_bSecondary = FALSE;

    pIOHeaderEnc = (U8 *)malloc((PACKETLENGTH * 4 - 1) + PACKETLENGTH * 4 + sizeof(BitIOInfo));
    if(pIOHeaderEnc == NULL)
        return ICERR_ERROR;
    memset(pIOHeaderEnc, 0, (PACKETLENGTH * 4 - 1) + PACKETLENGTH * 4 + sizeof(BitIOInfo));
    pSCEnc->pIOHeader = (BitIOInfo *)((U8 *)ALIGNUP(pIOHeaderEnc, PACKETLENGTH * 4) + PACKETLENGTH * 2);
    
    for(i = 0; i < pSCEnc->m_param.cNumChannels; i ++)
        pSCEnc->pPlane[i] = pSCDec->p1MBbuffer[i];
    
    for(i = 1; i < pSCDec->cNumBitIO * (pSCDec->WMISCP.cNumOfSliceMinus1H + 1); i ++){
        if(pSCDec->pIndexTable[i] == 0 && i + 1 != pSCDec->cNumBitIO * (pSCDec->WMISCP.cNumOfSliceMinus1H + 1)) // empty packet
            pSCDec->pIndexTable[i] = pSCDec->pIndexTable[i + 1];
        if(pSCDec->pIndexTable[i] != 0 && pSCDec->pIndexTable[i] < pSCDec->pIndexTable[i - 1]) // out of order bitstream, can not do fast tile extraction!
            pParam->bIgnoreOverlap = FALSE;
    }

    if(getROI(&pSCEnc->WMII, &pSCEnc->m_param, &pSCEnc->WMISCP, pParam) != ICERR_OK)
        return ICERR_ERROR;

    mbLeft = (pParam->cLeftX >> 4);
    mbRight = ((pParam->cLeftX + pParam->cWidth + 15) >> 4);
    mbTop = (pParam->cTopY >> 4);
    mbBottom = ((pParam->cTopY + pParam->cHeight + 15) >> 4);

    if(pSCDec->WMISCP.uiTileX[pSCDec->WMISCP.cNumOfSliceMinus1V] >= mbLeft && pSCDec->WMISCP.uiTileX[pSCDec->WMISCP.cNumOfSliceMinus1V] <= mbRight &&
        pSCDec->WMISCP.uiTileY[pSCDec->WMISCP.cNumOfSliceMinus1H] >= mbTop && pSCDec->WMISCP.uiTileY[pSCDec->WMISCP.cNumOfSliceMinus1H] <= mbBottom)
        pParam->bIgnoreOverlap = FALSE;

    pSCEnc->bTileExtraction = pParam->bIgnoreOverlap;

    mbWidth = pSCEnc->cmbWidth = mbRight - mbLeft;
    mbHeight = pSCEnc->cmbHeight = mbBottom - mbTop;
    if(orientation.transpose){
        JxrTranscodeSwapSize(&pSCEnc->WMII.cWidth, &pSCEnc->WMII.cHeight);
        JxrTranscodeSwapSize(&pSCEnc->cmbWidth, &pSCEnc->cmbHeight);
    }

    if(oO != O_NONE){
        pFrameBuf = (PixelI *)malloc(pSCEnc->cmbWidth * pSCEnc->cmbHeight * cUnit * sizeof(PixelI));
        if(pFrameBuf == NULL || (pSCEnc->cmbWidth * pSCEnc->cmbHeight * cUnit * sizeof(PixelI) < pSCEnc->cmbWidth * pSCEnc->cmbHeight * cUnit))
            return ICERR_ERROR;
        pMBInfo = (CWMIMBInfo *)malloc(pSCEnc->cmbWidth * pSCEnc->cmbHeight * sizeof(CWMIMBInfo));
        if(pMBInfo == NULL || (pSCEnc->cmbWidth * pSCEnc->cmbHeight * sizeof(CWMIMBInfo) < pSCEnc->cmbWidth * pSCEnc->cmbHeight))
            return ICERR_ERROR;
        if(pParam->uAlphaMode > 0){ // alpha channel
            pFrameBufAlpha = (PixelI *)malloc(pSCEnc->cmbWidth * pSCEnc->cmbHeight * 256 * sizeof(PixelI));
            if(pFrameBufAlpha == NULL || (pSCEnc->cmbWidth * pSCEnc->cmbHeight * 256 * sizeof(PixelI) < pSCEnc->cmbWidth * pSCEnc->cmbHeight * 256))
                return ICERR_ERROR;
            pMBInfoAlpha = (CWMIMBInfo *)malloc(pSCEnc->cmbWidth * pSCEnc->cmbHeight * sizeof(CWMIMBInfo));
            if(pMBInfoAlpha == NULL || (pSCEnc->cmbWidth * pSCEnc->cmbHeight * sizeof(CWMIMBInfo) < pSCEnc->cmbWidth * pSCEnc->cmbHeight))
                return ICERR_ERROR;
        }
    }

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

    if(pParam->uAlphaMode > 0){ // alpha channel
//        pSCEnc->WMISCP.nExpBias -= 128;
        if((pSCEnc->m_pNextSC = (CWMImageStrCodec *)malloc(sizeof(CWMImageStrCodec))) == NULL)
            return ICERR_ERROR;
        *pSCEnc->m_pNextSC = *pSCEnc;
        pSCEnc->m_pNextSC->pPlane[0] = pSCDec->m_pNextSC->p1MBbuffer[0];
        pSCEnc->m_pNextSC->WMISCP.cfColorFormat = pSCEnc->m_pNextSC->WMII.cfColorFormat = pSCEnc->m_pNextSC->m_param.cfColorFormat = Y_ONLY;
        pSCEnc->m_pNextSC->WMISCP.cChannel  = pSCEnc->m_pNextSC->m_param.cNumChannels = 1;
        pSCEnc->m_pNextSC->m_bSecondary = TRUE;
        pSCEnc->m_pNextSC->m_pNextSC = pSCEnc;
        pSCEnc->m_pNextSC->m_param = pSCDec->m_pNextSC->m_param;
        pSCEnc->m_param.bAlphaChannel = TRUE;

        if(pParam->bIgnoreOverlap == TRUE)
            pSCEnc->m_pNextSC->pTile = pSCDec->m_pNextSC->pTile;
        else if(StrEncInit(pSCEnc->m_pNextSC) != ICERR_OK)
                return ICERR_ERROR;

        WriteImagePlaneHeader(pSCEnc->m_pNextSC);
    }

    if(pParam->bIgnoreOverlap == TRUE){
        SUBBAND sbEnc = pSCEnc->WMISCP.sbSubband, sbDec = pSCDec->WMISCP.sbSubband;
        size_t cfEnc = ((pSCEnc->WMISCP.bfBitstreamFormat == SPATIAL || sbEnc == SB_DC_ONLY) ? 1 : (sbEnc == SB_NO_HIGHPASS ? 2 : (sbEnc == SB_NO_FLEXBITS ? 3 : 4)));
        size_t cfDec = ((pSCDec->WMISCP.bfBitstreamFormat == SPATIAL || sbDec == SB_DC_ONLY) ? 1 : (sbDec == SB_NO_HIGHPASS ? 2 : (sbDec == SB_NO_FLEXBITS ? 3 : 4)));
        size_t k, l = 0;

        pSCEnc->pIndexTable = (size_t *)malloc(sizeof(size_t) * (pSCEnc->WMISCP.cNumOfSliceMinus1H + 1) * (pSCEnc->WMISCP.cNumOfSliceMinus1V + 1) * cfEnc);

        if(pSCEnc->pIndexTable == NULL || cfEnc > cfDec)
            return ICERR_ERROR;

        pSCEnc->cNumBitIO = cfEnc * (pSCEnc->WMISCP.cNumOfSliceMinus1V + 1);
        
        for(j = 0; j <= pSCDec->WMISCP.cNumOfSliceMinus1H; j ++){
            for(i = 0; i <= pSCDec->WMISCP.cNumOfSliceMinus1V; i ++)
                if(pSCDec->WMISCP.uiTileX[i] >= mbLeft && pSCDec->WMISCP.uiTileX[i] < mbRight && 
                    pSCDec->WMISCP.uiTileY[j] >= mbTop && pSCDec->WMISCP.uiTileY[j] < mbBottom){
                        for(k = 0; k < cfEnc; k ++, l ++)
                            pSCEnc->pIndexTable[l] = pSCDec->pIndexTable[(j * (pSCDec->WMISCP.cNumOfSliceMinus1V + 1) + i) * cfDec + k + 1] - pSCDec->pIndexTable[(j * (pSCDec->WMISCP.cNumOfSliceMinus1V + 1) + i) * cfDec + k];
                }
        }

        if(pSCEnc->WMISCP.cNumOfSliceMinus1H + pSCEnc->WMISCP.cNumOfSliceMinus1V == 0 && pSCEnc->WMISCP.bfBitstreamFormat == SPATIAL){
            pSCEnc->m_param.bIndexTable = FALSE;
            pSCEnc->cNumBitIO = 0;
            writeIndexTableNull(pSCEnc);
        }
        else
            writeIndexTable(pSCEnc);
                
        detachISWrite(pSCEnc, pSCEnc->pIOHeader);

        for(j = l = 0; j <= pSCDec->WMISCP.cNumOfSliceMinus1H; j ++){
            for(i = 0; i <= pSCDec->WMISCP.cNumOfSliceMinus1V; i ++)
                if(pSCDec->WMISCP.uiTileX[i] >= mbLeft && pSCDec->WMISCP.uiTileX[i] < mbRight && 
                    pSCDec->WMISCP.uiTileY[j] >= mbTop && pSCDec->WMISCP.uiTileY[j] < mbBottom){
                        for(k = 0; k < cfEnc; k ++){
                            pSCDec->WMISCP.pWStream->SetPos(pSCDec->WMISCP.pWStream, pSCDec->pIndexTable[(j * (pSCDec->WMISCP.cNumOfSliceMinus1V + 1) + i) * cfDec + k] + pSCDec->cHeaderSize);
                            copyTo(pSCDec->WMISCP.pWStream, pSCEnc->WMISCP.pWStream, pSCEnc->pIndexTable[l++]);
                        }
                }
        }

        free(pSCEnc->pIndexTable);
    }
    else
        writeIndexTableNull(pSCEnc);

    for(pSCDec->cRow = 0; pSCDec->cRow < mbBottom && pParam->bIgnoreOverlap == FALSE; pSCDec->cRow ++){
        for(pSCDec->cColumn = 0; pSCDec->cColumn < pSCDec->cmbWidth; pSCDec->cColumn ++){
            Int cRow = (Int)pSCDec->cRow, cColumn = (Int)pSCDec->cColumn;
            CWMITile * pTile;
            
            memset(pMBBuf, 0, sizeof(PixelI) * cUnit);
            if(pSCDec->m_param.bAlphaChannel){ // alpha channel
                memset(pSCDec->m_pNextSC->p1MBbuffer[0], 0, sizeof(PixelI) * 256);
                pSCDec->m_pNextSC->cRow = pSCDec->cRow;
                pSCDec->m_pNextSC->cColumn = pSCDec->cColumn;
            }

            // decode
            pSC = pSCDec;
            for(i = (pSCDec->m_param.bAlphaChannel ? 2 : 1); i > 0; i --){
                getTilePos(pSCDec, cColumn, cRow);
                if(i == 2){
                    pSCDec->m_pNextSC->cTileColumn = pSCDec->cTileColumn;
                    pSCDec->m_pNextSC->cTileRow = pSCDec->cTileRow;
                }
                
                if (JxrDecoderPacketPipelineReadCurrentMacroblock(pSCDec) != ICERR_OK)
                    return ICERR_ERROR;

                pContext = &pSCDec->m_pCodingContext[pSCDec->cTileColumn];
                
                if(DecodeMacroblockDC(pSCDec, pContext, cColumn, cRow) != ICERR_OK)
                    return ICERR_ERROR;
                
                if(pSCDec->cSB > 1)
                    if(DecodeMacroblockLowpass(pSCDec, pContext, cColumn, cRow) != ICERR_OK)
                        return ICERR_ERROR;

                JxrDecoderCoefficientPredictorApplyDcAc(pSCDec);

                if(pSCDec->cSB > 2)
                    if(DecodeMacroblockHighpass(pSCDec, pContext, cColumn, cRow) != ICERR_OK)
                        return ICERR_ERROR;

                JxrDecoderCoefficientPredictorApplyAc(pSCDec);
                
                updatePredInfo(pSCDec, &pSCDec->MBInfo, cColumn, pSCDec->WMISCP.cfColorFormat);

                pSCDec = pSCDec->m_pNextSC;
            }
            pSCDec = pSC;

            if(pSCDec->cRow >= mbTop && pSCDec->cColumn >= mbLeft && pSCDec->cColumn < mbRight){
                cRow = (Int)(pSCDec->cRow - mbTop);
                cRow = JxrTranscodeOrientationStateMapRow(&orientation, cRow, mbHeight);
                cColumn = (Int)(pSCDec->cColumn - mbLeft);
                cColumn = JxrTranscodeOrientationStateMapColumn(&orientation, cColumn, mbWidth);

                pSCEnc->m_bCtxLeft = pSCEnc->m_bCtxTop = FALSE;
                for(i = 0; i <= pSCEnc->WMISCP.cNumOfSliceMinus1H; i ++)
                    if(pSCEnc->WMISCP.uiTileY[i] == (U32)JxrTranscodeOrientationStateTileRowCoordinate(&orientation, cRow, cColumn)){
                        pSCEnc->cTileRow = i;
                        pSCEnc->m_bCtxTop = TRUE;
                        break;
                    }
                for(i = 0; i <= pSCEnc->WMISCP.cNumOfSliceMinus1V; i ++)
                    if(pSCEnc->WMISCP.uiTileX[i] == (U32)JxrTranscodeOrientationStateTileColumnCoordinate(&orientation, cRow, cColumn)){
                        pSCEnc->cTileColumn = i;
                        pSCEnc->m_bCtxLeft = TRUE;
                        break;
                    }

                if(pSCEnc->m_bCtxLeft && pSCEnc->m_bCtxTop){ // a new tile, buffer tile DQuant info
                    JxrTranscodeTileQuantizerState * pTmp = pTileQPInfo;
                    
                    pTile = pSCDec->pTile + pSCDec->cTileColumn;
                    
                    if(oO != O_NONE)
                        pTmp += pSCEnc->cTileRow * (pSCEnc->WMISCP.cNumOfSliceMinus1V + 1) + pSCEnc->cTileColumn;
                    
                    JxrTranscodeTileQuantizerStateInit(pTmp);
                    JxrTranscodeTileQuantizerStateCapturePrimary(pTmp, pTile,
                        pSCEnc->WMISCP.cChannel, pSCEnc->WMISCP.sbSubband);
                    if(pParam->uAlphaMode > 0){
                        pTile = pSCDec->m_pNextSC->pTile + pSCDec->cTileColumn;
                        JxrTranscodeTileQuantizerStateCaptureAlpha(pTmp, pTile, iAlphaPos,
                            pSCEnc->WMISCP.sbSubband);
                    }
                }

                if(oO == O_NONE){
                    // encode
                    pSCEnc->cColumn = pSCDec->cColumn - mbLeft + 1;
                    pSCEnc->cRow = pSCDec->cRow + 1 - mbTop;
                    pSCEnc->MBInfo = pSCDec->MBInfo;

                    getTilePos(pSCEnc, cColumn, cRow);
                    
                    if(pSCEnc->m_bCtxLeft && pSCEnc->m_bCtxTop)
                        transcodeTileHeader(pSCEnc, pTileQPInfo);

                    if(encodeMB(pSCEnc, cColumn, cRow) != ICERR_OK)
                        return ICERR_ERROR;
                    if(pParam->uAlphaMode > 0){
                        pSCEnc->m_pNextSC->cColumn = pSCDec->cColumn - mbLeft + 1;
                        pSCEnc->m_pNextSC->cRow = pSCDec->cRow + 1 - mbTop;
                        getTilePos(pSCEnc->m_pNextSC, cColumn, cRow);
                        pSCEnc->m_pNextSC->MBInfo = pSCDec->m_pNextSC->MBInfo;
                        if(encodeMB(pSCEnc->m_pNextSC, cColumn, cRow) != ICERR_OK)
                            return ICERR_ERROR;
                    }
                }
                else{
                    size_t cOff = JxrTranscodeOrientationStateFrameOffset(&orientation, cRow, cColumn, mbWidth, mbHeight);

                    pMBInfo[cOff] = pSCDec->MBInfo;

                    memcpy(&pFrameBuf[cOff * cUnit], pMBBuf, cUnit * sizeof(PixelI));

                    if(pParam->uAlphaMode > 0){
                        pMBInfoAlpha[cOff] = pSCDec->m_pNextSC->MBInfo;
                        
                        memcpy(&pFrameBufAlpha[cOff * 256], MBBufAlpha, 256 * sizeof(PixelI));
                    }
                }
            }
        }

        advanceOneMBRow(pSCDec);

        if(oO == O_NONE)
            advanceOneMBRow(pSCEnc);
    }

    if(oO != O_NONE){
        for(pSCEnc->cRow = 1; pSCEnc->cRow <= pSCEnc->cmbHeight; pSCEnc->cRow ++){
            for(pSCEnc->cColumn = 1; pSCEnc->cColumn <= pSCEnc->cmbWidth; pSCEnc->cColumn ++){
                Int cRow, cColumn;
                size_t cOff = (pSCEnc->cRow - 1) * pSCEnc->cmbWidth + pSCEnc->cColumn - 1;
                
                for(i = 0; i < ((pSCEnc->m_param.cfColorFormat == YUV_420 || pSCEnc->m_param.cfColorFormat == YUV_422) ? 1 : pSCEnc->m_param.cNumChannels); i ++){
                    JxrTranscodeCoefficientBuffer sourceDc, destinationDc, sourceAc, destinationAc;
                    JxrTranscodeCoefficientBufferInit(&sourceDc, pMBInfo[cOff].iBlockDC[i], 0, 16);
                    JxrTranscodeCoefficientBufferInit(&destinationDc, pSCEnc->MBInfo.iBlockDC[i], 0, 16);
                    JxrTranscodeCoefficientBufferInit(&sourceAc, pFrameBuf + cOff * cUnit + i * 256, 0, 256);
                    JxrTranscodeCoefficientBufferInit(&destinationAc, pMBBuf + 256 * i, 0, 256);
                    if(JxrTranscodeCoefficientTransformDc444(&sourceDc, &destinationDc, &orientation) == FALSE ||
                        JxrTranscodeCoefficientTransformAc444(&sourceAc, &destinationAc, &orientation) == FALSE)
                        return ICERR_ERROR;
                }
                if(pSCEnc->WMISCP.cfColorFormat == YUV_420)
                    for(i = 0; i < 2; i ++){
                        JxrTranscodeCoefficientBuffer sourceDc, destinationDc, sourceAc, destinationAc;
                        JxrTranscodeCoefficientBufferInit(&sourceDc, pMBInfo[cOff].iBlockDC[i + 1], 0, 4);
                        JxrTranscodeCoefficientBufferInit(&destinationDc, pSCEnc->MBInfo.iBlockDC[i + 1], 0, 4);
                        JxrTranscodeCoefficientBufferInit(&sourceAc, pFrameBuf + cOff * cUnit + 256 + i * 64, 0, 64);
                        JxrTranscodeCoefficientBufferInit(&destinationAc, pMBBuf + 256 + i * 64, 0, 64);
                        if(JxrTranscodeCoefficientTransformDc420(&sourceDc, &destinationDc, &orientation) == FALSE ||
                            JxrTranscodeCoefficientTransformAc420(&sourceAc, &destinationAc, &orientation) == FALSE)
                            return ICERR_ERROR;
                    }
                else if(pSCEnc->WMISCP.cfColorFormat == YUV_422)
                    for(i = 0; i < 2; i ++){
                        JxrTranscodeCoefficientBuffer sourceDc, destinationDc, sourceAc, destinationAc;
                        JxrTranscodeCoefficientBufferInit(&sourceDc, pMBInfo[cOff].iBlockDC[i + 1], 0, 8);
                        JxrTranscodeCoefficientBufferInit(&destinationDc, pSCEnc->MBInfo.iBlockDC[i + 1], 0, 8);
                        JxrTranscodeCoefficientBufferInit(&sourceAc, pFrameBuf + cOff * cUnit + 256 + i * 128, 0, 128);
                        JxrTranscodeCoefficientBufferInit(&destinationAc, pMBBuf + 256 + i * 128, 0, 128);
                        if(JxrTranscodeCoefficientTransformDc422(&sourceDc, &destinationDc, &orientation) == FALSE ||
                            JxrTranscodeCoefficientTransformAc422(&sourceAc, &destinationAc, &orientation) == FALSE)
                            return ICERR_ERROR;
                    }

                    pSCEnc->MBInfo.iQIndexLP = pMBInfo[cOff].iQIndexLP;
                    pSCEnc->MBInfo.iQIndexHP = pMBInfo[cOff].iQIndexHP;

                cRow = (Int)pSCEnc->cRow - 1;
                cColumn = (Int)pSCEnc->cColumn - 1;
                getTilePos(pSCEnc, cColumn, cRow);

                if(pSCEnc->m_bCtxLeft && pSCEnc->m_bCtxTop)
                    transcodeTileHeader(pSCEnc, pTileQPInfo + pSCEnc->cTileRow * (pSCEnc->WMISCP.cNumOfSliceMinus1V + 1) + pSCEnc->cTileColumn);
                if(encodeMB(pSCEnc, cColumn, cRow) != ICERR_OK)
                    return ICERR_ERROR;
                
                if(pParam->uAlphaMode > 0){
                    pSCEnc->m_pNextSC->cColumn = pSCEnc->cColumn;
                    pSCEnc->m_pNextSC->cRow = pSCEnc->cRow;
                    getTilePos(pSCEnc->m_pNextSC, cColumn, cRow);
                    pSCEnc->m_pNextSC->MBInfo = pSCDec->m_pNextSC->MBInfo;

                    JxrTranscodeCoefficientBuffer sourceDc, destinationDc, sourceAc, destinationAc;
                    JxrTranscodeCoefficientBufferInit(&sourceDc, pMBInfoAlpha[cOff].iBlockDC[0], 0, 16);
                    JxrTranscodeCoefficientBufferInit(&destinationDc, pSCEnc->m_pNextSC->MBInfo.iBlockDC[0], 0, 16);
                    JxrTranscodeCoefficientBufferInit(&sourceAc, pFrameBufAlpha + cOff * 256, 0, 256);
                    JxrTranscodeCoefficientBufferInit(&destinationAc, MBBufAlpha, 0, 256);
                    if(JxrTranscodeCoefficientTransformDc444(&sourceDc, &destinationDc, &orientation) == FALSE ||
                        JxrTranscodeCoefficientTransformAc444(&sourceAc, &destinationAc, &orientation) == FALSE)
                        return ICERR_ERROR;

                    pSCEnc->m_pNextSC->MBInfo.iQIndexLP = pMBInfoAlpha[cOff].iQIndexLP;
                    pSCEnc->m_pNextSC->MBInfo.iQIndexHP = pMBInfoAlpha[cOff].iQIndexHP;

                    if(encodeMB(pSCEnc->m_pNextSC, cColumn, cRow) != ICERR_OK)
                        return ICERR_ERROR;
                }
            }

            advanceOneMBRow(pSCEnc);
        }
    }

    free(pMBBuf);
    if(oO != O_NONE){
        free(pFrameBuf);
        free(pMBInfo);
        if(pParam->uAlphaMode > 0){ // alpha channel
            free(pFrameBufAlpha);
            free(pMBInfoAlpha);
        }
    }

    freePredInfo(pSCDec);
    freeTileInfo(pSCDec);
    JxrDecoderResourceInitializerReleaseIo(pSCDec);
    FreeCodingContextDec(pSCDec);
    if(pSCDec->m_param.bAlphaChannel)
        free(pSCDec->m_pNextSC);
    free(pSCDec);
    free(pIOHeaderDec);

    if(pParam->bIgnoreOverlap == FALSE){
        freePredInfo(pSCEnc);
        freeTileInfo(pSCEnc);
        StrIOEncTerm(pSCEnc);
        free(pTileQPInfo);
        FreeCodingContextEnc(pSCEnc);
    }
    free(pSCEnc);
    free(pIOHeaderEnc);

    return ICERR_OK;
}
