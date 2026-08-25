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


static void fixup_Y_ONLY_to_Others(
    const CWMImageStrCodec* pSC,
    const CWMImageBufferInfo* pBI)
{
    const CWMImageInfo* pII = &pSC->WMII;
    const CWMIStrCodecParam* pSCP = &pSC->WMISCP;
    size_t cWidth = 0, cHeight = 0;

    if (CF_RGB != pII->cfColorFormat || Y_ONLY != pSCP->cfColorFormat)
        return;

    cWidth = 0 != pII->cROIWidth ? pII->cROIWidth : pII->cWidth;
    cHeight = 0 != pII->cROIHeight ? pII->cROIHeight : pII->cHeight;

    switch (pII->bdBitDepth)
    {
        case BD_8:
            JxrMonochromeExpansionReplicateByte((U8*)pBI->pv, pBI->cbStride, cWidth, cHeight, pII->cBitsPerUnit >> 3);
            break;

        case BD_16:
        case BD_16S:
        case BD_16F:
            JxrMonochromeExpansionReplicateUInt16((U16*)pBI->pv, pBI->cbStride, cWidth, cHeight, (pII->cBitsPerUnit >> 3) / sizeof(U16));
            break;

        case BD_32:
        case BD_32S:
        case BD_32F:
            JxrMonochromeExpansionReplicateUInt32((U32*)pBI->pv, pBI->cbStride, cWidth, cHeight, (pII->cBitsPerUnit >> 3) / sizeof(float));
            break;

        case BD_5:
        case BD_10:
        case BD_565:
        default:
            break;
    }
}

// Index-table syntax is implemented by JxrIndexTableReader.

static Bool JxrIndexTableReaderStoreLegacyEntry(Void* context, U32 index, U64 value)
{
    ((size_t*)context)[index] = (size_t)value;
    return TRUE;
}

//================================================================
Int readIndexTable(CWMImageStrCodec * pSC)
{
    BitIOInfo* pIO = pSC->pIOHeader;
    JxrIndexTableReader reader;
    JxrIndexTableLegacyContext legacyContext;
    U64 headerSize;
    U32 entryCount = (U32)pSC->cNumBitIO * (pSC->WMISCP.cNumOfSliceMinus1H + 1);

    legacyContext.codec = pSC;
    legacyContext.input = pIO;
    JxrIndexTableReaderInitLegacy(&reader, &legacyContext);
    if (!JxrIndexTableReaderRead(&reader, entryCount,
        entryCount == 0 ? NULL : JxrIndexTableReaderStoreLegacyEntry,
        pSC->pIndexTable, &headerSize))
    {
        return ICERR_ERROR;
    }

    pSC->cHeaderSize = (size_t)headerSize;
    return ICERR_OK;
}

static Int JxrDecoderStreamInitializeLegacyBitIo(Void* context)
{
    return allocateBitIOInfo((CWMImageStrCodec*)context);
}

static Int JxrDecoderStreamInitializeLegacyInput(Void* context)
{
    CWMImageStrCodec* codec = (CWMImageStrCodec*)context;
    return attachISRead(codec->pIOHeader, codec->WMISCP.pWStream, codec) == WMP_errSuccess ?
        ICERR_OK : ICERR_ERROR;
}

static Int JxrDecoderStreamInitializeLegacyIndexTable(Void* context)
{
    return readIndexTable((CWMImageStrCodec*)context);
}

Int StrIODecInit(CWMImageStrCodec* pSC)
{
    JxrDecoderStreamInitializer initializer;

    JxrDecoderStreamInitializerInit(&initializer, pSC,
        JxrDecoderStreamInitializeLegacyBitIo,
        JxrDecoderStreamInitializeLegacyInput,
        JxrDecoderStreamInitializeLegacyIndexTable);
    if (JxrDecoderStreamInitializerRun(&initializer) != ICERR_OK)
    {
        return ICERR_ERROR;
    }

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
                    size_t * p = &pSC->pIndexTable[j * (pSC->WMISCP.cNumOfSliceMinus1V + 1) + i];
                    if(i + j != pSC->WMISCP.cNumOfSliceMinus1H + pSC->WMISCP.cNumOfSliceMinus1V){
                        printf("bitstream size for tile (%d, %d): %d.\n", j, i, (int) (p[1] - p[0]));
                    }
                    else{
                        printf("bitstream size for tile (%d, %d): unknown.\n", j, i);
                    }
                }
            }
        }
        else{
            for(j = 0; j <= pSC->WMISCP.cNumOfSliceMinus1H; j ++){
                for(i = 0; i <= pSC->WMISCP.cNumOfSliceMinus1V; i ++){
                    size_t * p = &pSC->pIndexTable[(j * (pSC->WMISCP.cNumOfSliceMinus1V + 1) + i) * 4];
                    if(i + j != pSC->WMISCP.cNumOfSliceMinus1H + pSC->WMISCP.cNumOfSliceMinus1V){
                        printf("bitstream size of (DC, LP, AC, FL) for tile (%d, %d): %d %d %d %d.\n", j, i,
                            (int) (p[1] - p[0]), (int) (p[2] - p[1]), (int) (p[3] - p[2]), (int) (p[4] - p[3]));
                    }
                    else{
                        printf("bitstream size of (DC, LP, AC, FL) for tile (%d, %d): %d %d %d unknown.\n", j, i,
                            (int) (p[1] - p[0]), (int) (p[2] - p[1]), (int) (p[3] - p[2]));
                    }
                }
            }
        }
    }

    return 0;
}

Int StrIODecTerm(CWMImageStrCodec* pSC)
{
    detachISRead(pSC, pSC->pIOHeader);

    free(pSC->m_ppBitIO);
    free(pSC->pIndexTable);

    return 0;
}

Int initLookupTables(CWMImageStrCodec* pSC)
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

Void setROI(CWMImageStrCodec* pSC)
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

Int StrDecInit(CWMImageStrCodec* pSC)
{
    // CWMImageInfo * pWMII = &pSC->WMII;
    COLORFORMAT cfInt = pSC->m_param.cfColorFormat;
    COLORFORMAT cfExt = pSC->WMII.cfColorFormat;
    size_t i;

    /** color transcoding with resolution change **/
    pSC->m_bUVResolutionChange = ((cfExt != Y_ONLY) && ((cfInt == YUV_420 && cfExt != YUV_420) || 
        (cfInt == YUV_422 && cfExt != YUV_422))) && !pSC->WMISCP.bYUVData;
    if(pSC->m_bUVResolutionChange){
        pSC->pResU = (PixelI *)malloc((cfExt == YUV_422 ? 128 : 256) * pSC->cmbWidth * sizeof(PixelI));
        pSC->pResV = (PixelI *)malloc((cfExt == YUV_422 ? 128 : 256) * pSC->cmbWidth * sizeof(PixelI));
        if(pSC->pResU == NULL || pSC->pResV == NULL || (cfExt == YUV_422 ? 128 : 256) * pSC->cmbWidth * sizeof(PixelI) < pSC->cmbWidth){
            return ICERR_ERROR;
        }
    }

    if(allocatePredInfo(pSC) != ICERR_OK){
        return ICERR_ERROR;
    }

    if(allocateTileInfo(pSC) != ICERR_OK)
        return ICERR_ERROR;

    if((pSC->m_param.uQPMode & 1) == 0){ // DC frame uniform quantization
        if(allocateQuantizer(pSC->pTile[0].pQuantizerDC, pSC->m_param.cNumChannels, 1) != ICERR_OK)
            return ICERR_ERROR;
        setUniformQuantizer(pSC, 0);
        for(i = 0; i < pSC->m_param.cNumChannels; i ++)
            pSC->pTile[0].pQuantizerDC[i]->iIndex = pSC->m_param.uiQPIndexDC[i];
        formatQuantizer(pSC->pTile[0].pQuantizerDC, (pSC->m_param.uQPMode >> 3) & 3, pSC->m_param.cNumChannels, 0, TRUE, pSC->m_param.bScaledArith);
    }

    if(pSC->WMISCP.sbSubband != SB_DC_ONLY){
        if((pSC->m_param.uQPMode & 2) == 0){ // LP frame uniform quantization
            if(allocateQuantizer(pSC->pTile[0].pQuantizerLP, pSC->m_param.cNumChannels, 1) != ICERR_OK)
                return ICERR_ERROR;
            setUniformQuantizer(pSC, 1);
            if((pSC->m_param.uQPMode & 0x200) == 0) // use DC quantizer
                useDCQuantizer(pSC, 0);
            else{
                for(i = 0; i < pSC->m_param.cNumChannels; i ++)
                    pSC->pTile[0].pQuantizerLP[i]->iIndex = pSC->m_param.uiQPIndexLP[i];
                formatQuantizer(pSC->pTile[0].pQuantizerLP, (pSC->m_param.uQPMode >> 5) & 3, pSC->m_param.cNumChannels, 0, TRUE, pSC->m_param.bScaledArith);
            }
        }

        if(pSC->WMISCP.sbSubband != SB_NO_HIGHPASS){
            if((pSC->m_param.uQPMode & 4) == 0){ // HP frame uniform quantization
                if(allocateQuantizer(pSC->pTile[0].pQuantizerHP, pSC->m_param.cNumChannels, 1) != ICERR_OK)
                    return ICERR_ERROR;
                setUniformQuantizer(pSC, 2);

                if((pSC->m_param.uQPMode & 0x400) == 0) // use LP quantizer
                    useLPQuantizer(pSC, 1, 0);
                else{
                    for(i = 0; i < pSC->m_param.cNumChannels; i ++)
                        pSC->pTile[0].pQuantizerHP[i]->iIndex = pSC->m_param.uiQPIndexHP[i];
                    formatQuantizer(pSC->pTile[0].pQuantizerHP, (pSC->m_param.uQPMode >> 7) & 3, pSC->m_param.cNumChannels, 0, FALSE, pSC->m_param.bScaledArith);
                }
            }
        }
    }

    if(pSC->WMISCP.cNumOfSliceMinus1V >= MAX_TILES || AllocateCodingContextDec(pSC, pSC->WMISCP.cNumOfSliceMinus1V + 1) != ICERR_OK){
        return ICERR_ERROR;
    }

    if (pSC->m_bSecondary) {
        pSC->pIOHeader = pSC->m_pNextSC->pIOHeader;
        pSC->m_ppBitIO = pSC->m_pNextSC->m_ppBitIO;
        pSC->cNumBitIO = pSC->m_pNextSC->cNumBitIO;
        pSC->cSB = pSC->m_pNextSC->cSB;
    }

    setBitIOPointers(pSC);

    return ICERR_OK;
}

Int StrDecTerm(CWMImageStrCodec* pSC)
{
    size_t j, jend = (pSC->m_pNextSC != NULL);

    for (j = 0; j <= jend; j++) {
        if(pSC->m_bUVResolutionChange){        
            if(pSC->pResU != NULL)
                free(pSC->pResU);
            if(pSC->pResV != NULL)
                free(pSC->pResV);
        }

        freePredInfo(pSC);

        freeTileInfo(pSC);

        FreeCodingContextDec(pSC);

        if (j == 0) {
            StrIODecTerm(pSC);

            // free lookup tables for rotation and flipping
            if(pSC->m_Dparam->pOffsetX != NULL)
                free(pSC->m_Dparam->pOffsetX);
            if(pSC->m_Dparam->pOffsetY != NULL)
                free(pSC->m_Dparam->pOffsetY);
        }

        pSC = pSC->m_pNextSC;
    }

    return 0;
}

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
  Initialization of CWMImageStrCodec struct
*************************************************************************/
static Void InitializeStrDec(CWMImageStrCodec *pSC,
  const CCoreParameters *pParams, const CWMImageStrCodec *pSCIn)
{
    // copy core parameters
    memcpy (&(pSC->m_param), pParams, sizeof (CCoreParameters));

    pSC->cbStruct = sizeof(*pSC);
    pSC->WMII = pSCIn->WMII;
    pSC->WMISCP = pSCIn->WMISCP;

    pSC->cRow = 0;
    pSC->cColumn = 0;
    
    pSC->cmbWidth = (pSC->WMII.cWidth + 15) / 16;
    pSC->cmbHeight = (pSC->WMII.cHeight + 15) / 16;

#if defined(WMP_OPT_SSE2) || defined(WMP_OPT_CC_DEC) || defined(WMP_OPT_TRFM_DEC)
    pSC->Load = JxrDecoderOutputPipelineWriteStandardRow; // compatibility fallback for optimized output
#endif
    JxrDecoderTransformPipelineInitialize(pSC,
        pParams->cSubVersion != CODEC_SUBVERSION);


    pSC->m_pNextSC = NULL;
    pSC->m_bSecondary = FALSE;
}

static Void JxrLegacySecondaryPlaneInitializeCodec(CWMImageStrCodec* codec,
    const CCoreParameters* parameters, const CWMImageStrCodec* templateCodec)
{
    InitializeStrDec(codec, parameters, templateCodec);
}

static Int JxrLegacySecondaryPlaneReadHeader(CWMImageInfo* imageInfo,
    CWMIStrCodecParam* codecParameters, CCoreParameters* coreParameters,
    SimpleBitIO* bitInput)
{
    return ReadImagePlaneHeader(imageInfo, codecParameters, coreParameters, bitInput);
}
/*************************************************************************
  ImageStrDecInit
*************************************************************************/
Int ImageStrDecInit(
    CWMImageInfo* pII,
    CWMIStrCodecParam *pSCP,
    CTXSTRCODEC* pctxSC)
{
    static size_t cbChannels[BD_MAX] = {2, 4};
    size_t cbChannel = 0, cblkChroma = 0;
    size_t cbMacBlockStride = 0, cbMacBlockChroma = 0, cMacBlock = 0;

    CWMImageStrCodec SC = {0};
    CWMImageStrCodec *pSC = NULL, *pNextSC = NULL;
    char* pb = NULL;
    size_t cb = 0, i;
    Bool bLossyTranscoding = FALSE;
    Bool bUseHardTileBoundaries = FALSE; //default is soft tile boundaries
    Bool bLessThan64Bit = sizeof(void *) < 8;

    *pctxSC = NULL;

    if(WMPhotoValidate(pII, pSCP) != ICERR_OK)
        return ICERR_ERROR;

    if(pSCP->sbSubband == SB_ISOLATED) // can not do anything with isolated bitstream
        return ICERR_ERROR;

    //================================================
    SC.WMISCP.pWStream = pSCP->pWStream;
    if (ReadWMIHeader(&SC.WMII, &SC.WMISCP, &SC.m_param) != ICERR_OK) {
        return ICERR_ERROR;
    }

    bUseHardTileBoundaries = SC.WMISCP.bUseHardTileBoundaries;
    if(SC.WMII.cfColorFormat == CMYK && pII->cfColorFormat == CF_RGB)
        bLossyTranscoding = TRUE;
    if(pSCP->cfColorFormat != CMYK && (pII->cfColorFormat == CMYK))
        return ICERR_ERROR;

    //================================================
    SC.WMISCP = *pSCP;
    SC.WMII   = *pII;

    // original image size
    SC.WMII.cWidth += SC.m_param.cExtraPixelsLeft + SC.m_param.cExtraPixelsRight;
    SC.WMII.cHeight += SC.m_param.cExtraPixelsTop + SC.m_param.cExtraPixelsBottom;
    pII->cROILeftX += SC.m_param.cExtraPixelsLeft;
    pII->cROITopY += SC.m_param.cExtraPixelsTop;
    
    //================================================
    cbChannel = cbChannels[SC.WMISCP.bdBitDepth];
    cblkChroma = cblkChromas[SC.m_param.cfColorFormat];

    cbMacBlockStride = cbChannel * 16 * 16;
    cbMacBlockChroma = cbChannel * 16 * cblkChroma;
    cMacBlock = (SC.WMII.cWidth + 15) / 16;

    //================================================
    cb = sizeof(*pSC) + (128 - 1) + sizeof(CWMDecoderParameters);
    cb += (PACKETLENGTH * 4 - 1) + (PACKETLENGTH * 2 ) + sizeof(*pSC->pIOHeader);

    i = (cbMacBlockStride + cbMacBlockChroma * (SC.m_param.cNumChannels - 1)) * 2; // i <= 2^15
    if (bLessThan64Bit && ((i * (cMacBlock >> 16)) & 0xffffc000)) {
        /** potential overflow - 32 bit pointers insufficient to address cache **/
        return ICERR_ERROR;
    }
    cb += i * cMacBlock;

    pb = malloc(cb);
    if(pb == NULL)
        return WMP_errOutOfMemory;
    memset(pb, 0, cb);

    //================================================
    pSC = (CWMImageStrCodec*)pb; pb += sizeof(*pSC);
    if(pSC == NULL)
        return ICERR_ERROR;

    // Set up perf timers
    PERFTIMER_ONLY(pSC->m_fMeasurePerf = pSCP->fMeasurePerf);
    PERFTIMER_NEW(pSC->m_fMeasurePerf, &pSC->m_ptEndToEndPerf);
    PERFTIMER_NEW(pSC->m_fMeasurePerf, &pSC->m_ptEncDecPerf);
    PERFTIMER_START(pSC->m_fMeasurePerf, pSC->m_ptEndToEndPerf);
    PERFTIMER_START(pSC->m_fMeasurePerf, pSC->m_ptEncDecPerf);
    PERFTIMER_COPYSTARTTIME(pSC->m_fMeasurePerf, pSC->m_ptEncDecPerf, pSC->m_ptEndToEndPerf);

    pSC->m_Dparam = (CWMDecoderParameters*)pb; pb += sizeof(CWMDecoderParameters);
    pSC->cbChannel = cbChannel;
    //pSC->cNumChannels = SC.WMISCP.cChannel;
    pSC->bUseHardTileBoundaries = bUseHardTileBoundaries;

    //================================================
    InitializeStrDec(pSC, &SC.m_param, &SC);

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
    pSC->pIOHeader = (BitIOInfo*)pb; pb += sizeof(*pSC->pIOHeader);

    // Create and read the alpha plane before the shared decoder initialization pipeline.
    if (pSC->m_param.bAlphaChannel) {
        JxrSecondaryPlaneInitializer secondaryInitializer;
        Int secondaryResult;
        JxrSecondaryPlaneInitializerInit(&secondaryInitializer, pSC, &SC.m_param, &SC,
            cbChannel, cMacBlock, JxrLegacySecondaryPlaneInitializeCodec,
            JxrLegacySecondaryPlaneReadHeader);
        secondaryResult = JxrSecondaryPlaneInitializerRun(&secondaryInitializer, &pNextSC);
        if (secondaryResult != ICERR_OK)
            return secondaryResult;
    }
    else
        pSC->WMISCP.uAlphaMode = 0;
    //================================================
    {
        JxrDecoderInitializationPipeline initialization;
        JxrDecoderInitializationPipelineInit(&initialization, pSC, pNextSC,
            StrIODecInit, StrDecInit);
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
    CWMImageStrCodec* pNextSC = pSC->m_pNextSC;
    size_t cMBRow;
    JxrDecoderOutputPipelinePlan outputPipeline;
    if (sizeof(*pSC) != pSC->cbStruct)
    {
        return ICERR_ERROR;
    }

    //================================
    PERFTIMER_START(pSC->m_fMeasurePerf, pSC->m_ptEncDecPerf);

    pSC->WMIBI = *pBI;

#ifdef REENTRANT_MODE
    if (0 == pSC->WMIBI.uiFirstMBRow)
    {
        setROI(pSC);
        if (pNextSC) {
            pNextSC->WMIBI = pSC->WMIBI;
            setROI(pNextSC);
        }
    }
#else
    setROI(pSC);
    if (pNextSC) {
        pNextSC->WMIBI = pSC->WMIBI;
        setROI(pNextSC);
    }
#endif // REENTRANT_MODE

// optimization flags can be defined only after ROI is set!
#if defined(WMP_OPT_SSE2) || defined(WMP_OPT_CC_DEC) || defined(WMP_OPT_TRFM_DEC)
    StrDecOpt(pSC);
    JxrDecoderOutputPipelinePlanInitialize(&outputPipeline, TRUE);
#else
    JxrDecoderOutputPipelinePlanInitialize(&outputPipeline, FALSE);
#endif // OPT defined



    cMBRow = (pSC->m_Dparam->bDecodeFullFrame ? pSC->cmbHeight : ((pSC->m_Dparam->cROIBottomY + 16) >> 4));

#ifdef REENTRANT_MODE
    if (0 == pSC->WMIBI.uiFirstMBRow)
    {
        if(initLookupTables(pSC) != ICERR_OK)
            return ICERR_ERROR;
        if (pNextSC && initLookupTables(pNextSC) != ICERR_OK)
            return ICERR_ERROR;
    }
#else
    if(initLookupTables(pSC) != ICERR_OK)
        return ICERR_ERROR;
    if (pNextSC && initLookupTables(pNextSC) != ICERR_OK)
        return ICERR_ERROR;
#endif // REENTRANT_MODE

#ifndef REENTRANT_MODE
    if(pSC->WMII.bdBitDepth == BD_1){
        size_t i;


        for(i = 0; i < pSC->WMIBI.cLine; i ++)
            memset(pSC->WMIBI.pv, 0, pSC->WMIBI.cbStride);
    }
#endif

    if (JxrDecoderExecutionPipelineRun(pSC, cMBRow,
        outputPipeline.usesLegacyLoadCallback
#ifdef REENTRANT_MODE
        , pcDecodedLines
#endif
        ) != ICERR_OK)
        return ICERR_ERROR;
#ifndef REENTRANT_MODE
    fixup_Y_ONLY_to_Others(pSC, pBI);
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

    StrDecTerm(pSC);
    PERFTIMER_STOP(pSC->m_fMeasurePerf, pSC->m_ptEncDecPerf);
    PERFTIMER_REPORT(pSC->m_fMeasurePerf, pSC);
    PERFTIMER_DELETE(pSC->m_fMeasurePerf, pSC->m_ptEncDecPerf);
    PERFTIMER_DELETE(pSC->m_fMeasurePerf, pSC->m_ptEndToEndPerf);

    free(pSC);

    return ICERR_OK;
}

