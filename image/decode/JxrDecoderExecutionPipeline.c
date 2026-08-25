#include "JxrDecoderExecutionPipeline.h"

#include "decode.h"
#include "JxrDecoderMacroblockProcessingPipeline.h"
#include "JxrDecoderOutputPipeline.h"
#include "JxrDecoderTransformPipeline.h"

static Int JxrDecoderExecutionPipelineInitializeLookupTables(CWMImageStrCodec* pSC)
{
    static const U8 cbChannels[BDB_MAX] = {1, 1, 2, 2, 2, 4, 4, 4, (U8) -1, (U8) -1, (U8) -1 };

    CWMImageInfo * pII = &pSC->WMII;
    size_t cStrideX, cStrideY;
    size_t w, h, i, iFirst = 0;
    Bool bReverse;

    // lookup tables for rotation and flipping
    if(pSC->m_Dparam->cThumbnailScale > 1) // thumbnail
        w = pII->cThumbnailWidth, h = pII->cThumbnailHeight;
    else
        w = pII->cWidth, h = pII->cHeight;
    w += (pSC->m_Dparam->cROILeftX + pSC->m_Dparam->cThumbnailScale - 1) / pSC->m_Dparam->cThumbnailScale;
    h += (pSC->m_Dparam->cROITopY + pSC->m_Dparam->cThumbnailScale - 1) / pSC->m_Dparam->cThumbnailScale;

    switch(pII->bdBitDepth){
        case BD_16:
        case BD_16S:
        case BD_5:
        case BD_565:
        case BD_16F:
            cStrideY = pSC->WMIBI.cbStride / 2;
            break;

        case BD_32:
        case BD_32S:
        case BD_32F:
        case BD_10:
            cStrideY = pSC->WMIBI.cbStride / 4;
            break;

        default: //BD_8, BD_1
            cStrideY = pSC->WMIBI.cbStride;
            break;
    }

    switch(pII->cfColorFormat){
        case YUV_420:
            cStrideX = 6;
            w >>= 1, h >>= 1;
            break;

        case YUV_422:
            cStrideX = 4;
            w >>= 1;
            break;

        default:
            cStrideX = (pII->cBitsPerUnit >> 3) / cbChannels[pII->bdBitDepth];
            break;
    }

    if(pII->bdBitDepth == BD_1 || pII->bdBitDepth == BD_5 || pII->bdBitDepth == BD_10 ||  pII->bdBitDepth == BD_565)
        cStrideX = 1;

    if(pII->oOrientation > O_FLIPVH) // rotated !!
        i =cStrideX, cStrideX = cStrideY, cStrideY = i;

    pSC->m_Dparam->pOffsetX = (size_t *)malloc(w * sizeof(size_t));
    if(pSC->m_Dparam->pOffsetX == NULL || w * sizeof(size_t) < w)
        return ICERR_ERROR;
    /*
    consider a row in the source image. if it becomes a reversed row in the target, or a reversed (upside-down)column
    in the target, we have to reverse the offsets. bReverse here tells us when this happened.
    */
    bReverse = (pII->oOrientation == O_FLIPH || pII->oOrientation == O_FLIPVH ||
        pII->oOrientation == O_RCW_FLIPV || pII->oOrientation == O_RCW_FLIPVH);
    if(!pSC->m_Dparam->bDecodeFullFrame) // take care of region decode here!
        iFirst = (pSC->m_Dparam->cROILeftX + pSC->m_Dparam->cThumbnailScale - 1) / pSC->m_Dparam->cThumbnailScale;
    for(i = 0; i + iFirst < w; i ++){
        pSC->m_Dparam->pOffsetX[i + iFirst] = pII->cLeadingPadding + (bReverse ? (pSC->m_Dparam->bDecodeFullFrame ? w :
    (pSC->m_Dparam->cROIRightX - pSC->m_Dparam->cROILeftX + pSC->m_Dparam->cThumbnailScale) / pSC->m_Dparam->cThumbnailScale / ((pII->cfColorFormat == YUV_420 || pII->cfColorFormat == YUV_422) ? 2 : 1)) - 1 - i : i) * cStrideX;
    }

    pSC->m_Dparam->pOffsetY = (size_t *)malloc(h * sizeof(size_t));
    if(pSC->m_Dparam->pOffsetY == NULL || h * sizeof(size_t) < h)
        return ICERR_ERROR;
    /*
    consider a column in the source image. if it becomes an upside-down column in the target, or a reversed row
    in the target, we have to reverse the offsets. bReverse here tells us when this happened.
    */
    bReverse = (pII->oOrientation == O_FLIPV || pII->oOrientation == O_FLIPVH ||
        pII->oOrientation == O_RCW || pII->oOrientation == O_RCW_FLIPV);
    if(!pSC->m_Dparam->bDecodeFullFrame) // take care of region decode here!
        iFirst = (pSC->m_Dparam->cROITopY + pSC->m_Dparam->cThumbnailScale - 1) / pSC->m_Dparam->cThumbnailScale;
    for(i = 0; i + iFirst < h; i ++){
        pSC->m_Dparam->pOffsetY[i + iFirst] = (bReverse ? (pSC->m_Dparam->bDecodeFullFrame ? h :
    (pSC->m_Dparam->cROIBottomY - pSC->m_Dparam->cROITopY + pSC->m_Dparam->cThumbnailScale) / pSC->m_Dparam->cThumbnailScale / (pII->cfColorFormat == YUV_420 ? 2 : 1)) - 1 - i : i) * cStrideY;
    }

    return ICERR_OK;
}

static Void JxrDecoderExecutionPipelineSetRoi(CWMImageStrCodec* pSC)
{
    CWMImageInfo * pWMII = &pSC->WMII;
    CWMIStrCodecParam * pSCP = &pSC->WMISCP;

    // inscribed image size
    pWMII->cWidth -= pSC->m_param.cExtraPixelsLeft + pSC->m_param.cExtraPixelsRight;
    pWMII->cHeight -= pSC->m_param.cExtraPixelsTop + pSC->m_param.cExtraPixelsBottom;

    pSC->m_Dparam->bSkipFlexbits = (pSCP->sbSubband == SB_NO_FLEXBITS);
    pSC->m_Dparam->bDecodeHP = (pSCP->sbSubband == SB_ALL || pSCP->sbSubband == SB_NO_FLEXBITS);
    pSC->m_Dparam->bDecodeLP = (pSCP->sbSubband != SB_DC_ONLY);
    pSC->m_Dparam->cThumbnailScale = 1;
    while(pSC->m_Dparam->cThumbnailScale * pWMII->cThumbnailWidth < pWMII->cWidth)
        pSC->m_Dparam->cThumbnailScale <<= 1;
    if(pSC->WMISCP.bfBitstreamFormat == FREQUENCY){
        if(pSC->m_Dparam->cThumbnailScale >= 4)
            pSC->m_Dparam->bDecodeHP = FALSE;  // no need to decode HP
        if(pSC->m_Dparam->cThumbnailScale >= 16)
            pSC->m_Dparam->bDecodeLP = FALSE; // only need to decode DC
    }

    // original image size
    pWMII->cWidth += pSC->m_param.cExtraPixelsLeft + pSC->m_param.cExtraPixelsRight;
    pWMII->cHeight += pSC->m_param.cExtraPixelsTop + pSC->m_param.cExtraPixelsBottom;

    /** region decode stuff */
    pSC->m_Dparam->cROILeftX = pWMII->cROILeftX * pSC->m_Dparam->cThumbnailScale + pSC->m_param.cExtraPixelsLeft;
    pSC->m_Dparam->cROIRightX = pSC->m_Dparam->cROILeftX + pWMII->cROIWidth * pSC->m_Dparam->cThumbnailScale - 1;
    pSC->m_Dparam->cROITopY = pWMII->cROITopY * pSC->m_Dparam->cThumbnailScale + pSC->m_param.cExtraPixelsTop;
    pSC->m_Dparam->cROIBottomY = pSC->m_Dparam->cROITopY + pWMII->cROIHeight * pSC->m_Dparam->cThumbnailScale - 1;
    if(pSC->m_Dparam->cROIRightX >= pWMII->cWidth)
        pSC->m_Dparam->cROIRightX = pWMII->cWidth - 1;
    if(pSC->m_Dparam->cROIBottomY >= pWMII->cHeight)
        pSC->m_Dparam->cROIBottomY = pWMII->cHeight - 1;

    pSC->m_Dparam->bDecodeFullFrame = (pSC->m_Dparam->cROILeftX + pSC->m_Dparam->cROITopY == 0 &&
        ((pSC->m_Dparam->cROIRightX + 15) / 16 >= (pWMII->cWidth + 14) / 16) && ((pSC->m_Dparam->cROIBottomY + 15) / 16 >= (pWMII->cHeight + 14) / 16));

    pSC->m_Dparam->bDecodeFullWidth = (pSC->m_Dparam->cROILeftX == 0 && ((pSC->m_Dparam->cROIRightX + 15) / 16 >= (pWMII->cWidth + 14) / 16));

    // inscribed image size
    pWMII->cWidth -= pSC->m_param.cExtraPixelsLeft + pSC->m_param.cExtraPixelsRight;
    pWMII->cHeight -= pSC->m_param.cExtraPixelsTop + pSC->m_param.cExtraPixelsBottom;

    if(pSC->WMISCP.bfBitstreamFormat == FREQUENCY && pWMII->bSkipFlexbits == TRUE)
        pSC->m_Dparam->bSkipFlexbits = TRUE;

    pSC->cTileColumn = pSC->cTileRow = 0;
}


#if defined(WMP_OPT_SSE2) || defined(WMP_OPT_CC_DEC) || defined(WMP_OPT_TRFM_DEC)
void StrDecOpt(CWMImageStrCodec* codec);
#endif

Int JxrDecoderExecutionPipelinePrepare(CWMImageStrCodec* codec,
    const CWMImageBufferInfo* outputBuffer,
    JxrDecoderExecutionPreparation* preparation)
{
    CWMImageStrCodec* secondaryCodec = codec->m_pNextSC;

    codec->WMIBI = *outputBuffer;
#ifdef REENTRANT_MODE
    if (codec->WMIBI.uiFirstMBRow == 0) {
        JxrDecoderExecutionPipelineSetRoi(codec);
        if (secondaryCodec != NULL) {
            secondaryCodec->WMIBI = codec->WMIBI;
            JxrDecoderExecutionPipelineSetRoi(secondaryCodec);
        }
    }
#else
    JxrDecoderExecutionPipelineSetRoi(codec);
    if (secondaryCodec != NULL) {
        secondaryCodec->WMIBI = codec->WMIBI;
        JxrDecoderExecutionPipelineSetRoi(secondaryCodec);
    }
#endif

#if defined(WMP_OPT_SSE2) || defined(WMP_OPT_CC_DEC) || defined(WMP_OPT_TRFM_DEC)
    StrDecOpt(codec);
    preparation->usesLegacyLoadCallback = TRUE;
#else
    preparation->usesLegacyLoadCallback = FALSE;
#endif
    preparation->macroblockRowCount = codec->m_Dparam->bDecodeFullFrame ?
        codec->cmbHeight : ((codec->m_Dparam->cROIBottomY + 16) >> 4);

#ifdef REENTRANT_MODE
    if (codec->WMIBI.uiFirstMBRow == 0) {
        if (JxrDecoderExecutionPipelineInitializeLookupTables(codec) != ICERR_OK)
            return ICERR_ERROR;
        if (secondaryCodec != NULL && JxrDecoderExecutionPipelineInitializeLookupTables(secondaryCodec) != ICERR_OK)
            return ICERR_ERROR;
    }
#else
    if (JxrDecoderExecutionPipelineInitializeLookupTables(codec) != ICERR_OK)
        return ICERR_ERROR;
    if (secondaryCodec != NULL && JxrDecoderExecutionPipelineInitializeLookupTables(secondaryCodec) != ICERR_OK)
        return ICERR_ERROR;

    if (codec->WMII.bdBitDepth == BD_1) {
        size_t rowIndex;
        for (rowIndex = 0; rowIndex < codec->WMIBI.cLine; rowIndex++)
            memset(codec->WMIBI.pv, 0, codec->WMIBI.cbStride);
    }
#endif

    return ICERR_OK;
}

Int JxrDecoderExecutionPipelineRun(CWMImageStrCodec* codec, size_t macroblockRowCount,
    Bool usesLegacyLoadCallback
#ifdef REENTRANT_MODE
    , size_t* decodedLines
#endif
    )
{
    Bool useCenterTransform = FALSE;
    const size_t chromaElementCount = codec->m_param.cfColorFormat == YUV_420 ? 8 * 8 :
        (codec->m_param.cfColorFormat == YUV_422 ? 8 * 16 : 16 * 16);
    size_t channelIndex;

#ifdef REENTRANT_MODE
    for (codec->cRow = codec->WMIBI.uiFirstMBRow;
        codec->cRow <= codec->WMIBI.uiLastMBRow; codec->cRow++)
    {
        if (codec->cRow == 0 || codec->cRow == macroblockRowCount)
            useCenterTransform = FALSE;
        else
            useCenterTransform = TRUE;
#else
    codec->cRow = 0;
    for (codec->cRow = 0; codec->cRow <= macroblockRowCount; codec->cRow++)
    {
#endif
        codec->cColumn = 0;
        initMRPtr(codec);
        memset(codec->p1MBbuffer[0], 0,
            sizeof(PixelI) * 16 * 16 * codec->cmbWidth);
        for (channelIndex = 1; channelIndex < codec->m_param.cNumChannels; channelIndex++) {
            memset(codec->p1MBbuffer[channelIndex], 0,
                sizeof(PixelI) * chromaElementCount * codec->cmbWidth);
        }
        if (codec->m_pNextSC != NULL) {
            memset(codec->m_pNextSC->p1MBbuffer[0], 0,
                sizeof(PixelI) * 16 * 16 * codec->m_pNextSC->cmbWidth);
        }

        if (JxrDecoderMacroblockProcessingPipelineProcess(codec) != ICERR_OK)
            return ICERR_ERROR;
        advanceMRPtr(codec);

        JxrDecoderTransformPipelineSetCenterMacroblock(codec, useCenterTransform);
        for (codec->cColumn = 1; codec->cColumn < codec->cmbWidth; ++codec->cColumn) {
            if (JxrDecoderMacroblockProcessingPipelineProcess(codec) != ICERR_OK)
                return ICERR_ERROR;
            advanceMRPtr(codec);
        }

        JxrDecoderTransformPipelineSetCenterMacroblock(codec, FALSE);
        if (JxrDecoderMacroblockProcessingPipelineProcess(codec) != ICERR_OK)
            return ICERR_ERROR;

        if (codec->cRow != 0) {
            if (codec->m_Dparam->cThumbnailScale < 2 &&
                (codec->m_Dparam->bDecodeFullFrame ||
                (codec->cRow * 16 > codec->m_Dparam->cROITopY &&
                codec->cRow * 16 <= codec->m_Dparam->cROIBottomY + 16))) {
                if (usesLegacyLoadCallback) {
                    if (codec->Load(codec) != ICERR_OK)
                        return ICERR_ERROR;
                }
                else if (JxrDecoderOutputPipelineWriteStandardRow(codec) != ICERR_OK)
                    return ICERR_ERROR;
            }

            if (codec->m_Dparam->cThumbnailScale >= 2)
                JxrDecoderOutputPipelineWriteThumbnailRow(codec);
        }

        advanceOneMBRow(codec);
        swapMRPtr(codec);
#ifdef REENTRANT_MODE
        *decodedLines = codec->WMIBI.cLinesDecoded;
#else
        useCenterTransform = codec->cRow != macroblockRowCount - 1;
#endif
    }

    return ICERR_OK;
}
