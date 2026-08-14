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

#include "strTransform.h"
#include "strcodec.h"
#include "decode.h"
#include "JxrInverseTransformMath.h"
#include "JxrInverseTransformStages.h"
#include "JxrInverseTransformMacroblockGeometry.h"
#include "JxrHardTileBoundaryState.h"
#include "JxrInverseTransformBoundaryContext.h"
#include "JxrInversePostProcessParameters.h"
#include "JxrInverseHighPassParameters.h"
#include "JxrInverseTransformPlanePlan.h"
#include "JxrInverseTransformPlaneBuffers.h"
#include "JxrInverseTransformPlaneContext.h"
#include "JxrInverseTransformPlaneStage2.h"
#include "JxrInverseTransformPlaneStage2Normal.h"
#include "JxrInverseTransformPlaneStage2Alternate.h"
#include "JxrInverseTransformPlaneStage1Normal.h"
#include "JxrInverseTransformPlaneStage1Alternate.h"
#include "JxrInverseTransformChroma420Plane.h"
#include "JxrInverseTransformFullResolutionPlane.h"
#include "JxrInverseTransformAlternateFullResolutionPlane.h"
#include "JxrInverseTransformChroma422Plane.h"
#include "JxrInverseTransformChroma420AlternatePlane.h"
#include "JxrInverseTransformChroma422AlternatePlane.h"
#include "JxrTransformMath.h"

/** local functions **/

/** IDCT stuff **/
/** reordering should be combined with zigzag scan **/
/** data order before IDCT **/
/** 0  8  4  6 **/
/** 2 10 14 12 **/
/** 1 11 15 13 **/
/** 9  3  7  5 **/
/** data order after IDCT **/
/**  0  1  2  3 **/
/**  4  5  6  7 **/
/**  8  9 10 11 **/
/** 12 13 14 15 **/
Void strIDCT4x4Stage1(PixelI* p)
{
    JxrInverseTransformStagesApplyStage1Idct(p);
}

Void strIDCT4x4Stage2(PixelI* p)
{
    JxrInverseTransformStagesApplyStage2Idct(p);
}

Void strNormalizeDec(PixelI* p, Bool bChroma)
{
    int i;
    if (!bChroma) {
        //for (i = 0; i < 256; i += 16) {
        //    p[i] <<= 2;
        //}
    }
    else {
        for (i = 0; i < 256; i += 16) {
            p[i] += p[i];
        }
    }
}

/*****************************************************************************************
  Input data offsets:
  (15)(14)|(10+64)(11+64) p0 (15)(14)|(74)(75)
  (13)(12)|( 8+64)( 9+64)    (13)(12)|(72)(73)
  --------+--------------    --------+--------
  ( 5)( 4)|( 0+64) (1+64) p1 ( 5)( 4)|(64)(65)
  ( 7)( 6)|( 2+64) (3+64)    ( 7)( 6)|(66)(67)
*****************************************************************************************/
Void strPost4x4Stage1Split(PixelI *p0, PixelI *p1, Int iOffset, Int iHPQP, Bool bHPAbsent)
{
    JxrInverseTransformStagesApplyStage1SplitNormal(p0, p1, iOffset, iHPQP, bHPAbsent);
}

Void strPost4x4Stage1(PixelI* p, Int iOffset, Int iHPQP, Bool bHPAbsent)
{
    strPost4x4Stage1Split(p, p + 16, iOffset, iHPQP, bHPAbsent);
}

Void strPost4x4Stage1Split_alternate(PixelI *p0, PixelI *p1, Int iOffset)
{
    JxrInverseTransformStagesApplyStage1SplitAlternate(p0, p1, iOffset);
}

Void strPost4x4Stage1_alternate(PixelI* p, Int iOffset)
{
    strPost4x4Stage1Split_alternate(p, p + 16, iOffset);
}
/*****************************************************************************************
  Input data offsets:
  (15)(14)|(10+32)(11+32) p0 (15)(14)|(42)(43)
  (13)(12)|( 8+32)( 9+32)    (13)(12)|(40)(41)
  --------+--------------    --------+--------
  ( 5)( 4)|( 0+32) (1+32) p1 ( 5)( 4)|(32)(33)
  ( 7)( 6)|( 2+32) (3+32)    ( 7)( 6)|(34)(35)
*****************************************************************************************/

/*****************************************************************************************
  Input data offsets:
  ( -96)(-32)|(32)( 96) p0
  ( -80)(-16)|(48)(112)
  -----------+------------
  (-128)(-64)|( 0)( 64) p1
  (-112)(-48)|(16)( 80)
*****************************************************************************************/
Void strPost4x4Stage2Split(PixelI* p0, PixelI* p1)
{
    JxrInverseTransformStagesApplyStage2SplitNormal(p0, p1);
}

Void strPost4x4Stage2Split_alternate(PixelI* p0, PixelI* p1)
{
    JxrInverseTransformStagesApplyStage2SplitAlternate(p0, p1);
}
/*************************************************************************
  Top-level function to inverse tranform possible part of a macroblock
*************************************************************************/
Int  invTransformMacroblock(CWMImageStrCodec * pSC)
{
    JxrInverseTransformMacroblockGeometry geometry;
    JxrInverseTransformPlanePlan planePlan;
    // const BITDEPTH_BITS bdBitDepth = pSC->WMII.bdBitDepth;
    size_t i;
    JxrInverseTransformMacroblockGeometryInitialize(&geometry,
        pSC->WMISCP.olOverlap, pSC->m_param.cfColorFormat,
        pSC->cColumn, pSC->cRow, pSC->cmbWidth, pSC->cmbHeight,
        pSC->m_param.cNumChannels, pSC->m_Dparam->cThumbnailScale);
    const OVERLAP olOverlap = geometry.overlap;
    const COLORFORMAT cfColorFormat = geometry.colorFormat;
    const Bool left = geometry.isLeft;
    const Bool top = geometry.isTop, bottom = geometry.isBottom;
    const size_t mbWidth = geometry.macroblockWidth, mbX = geometry.macroblockColumn;
    const size_t iChannels = geometry.channelCount;
    const size_t tScale = geometry.thumbnailScale;
    JxrInverseTransformPlanePlanInitialize(&planePlan,
        cfColorFormat, iChannels,
        tScale);

    JxrInversePostProcessParameters postProcessParameters;
    // ERR_CODE result = ICERR_OK;

    JxrInverseHighPassParameters highPassParameters;

    JxrInversePostProcessParametersInitialize(&postProcessParameters,
        pSC->WMII.cPostProcStrength, olOverlap, iChannels,
        pSC->pTile[pSC->cTileColumn].pQuantizerLP,
        pSC->pTile[pSC->cTileColumn].pQuantizerDC,
        pSC->MBInfo.iQIndexLP);
    JxrInverseHighPassParametersInitialize(&highPassParameters,
        pSC->WMISCP.sbSubband, pSC->m_param.cNumChannels,
        pSC->pTile[pSC->cTileColumn].pQuantizerHP,
        pSC->MBInfo.iQIndexHP);
    if (postProcessParameters.enabled) {
        if (left) // a new MB row
            slideOneMBRow(pSC->pPostProcInfo, pSC->m_param.cNumChannels, mbWidth, top, bottom);  // previous current row becomes previous row
    }
    //================================================================
    // 400_Y, 444_YUV
    for (i = 0; i < planePlan.fullResolutionChannelCount && planePlan.transformsSamples; ++i)
    {
        JxrInverseTransformPlaneContext planeContext;
        JxrInverseTransformPlaneContextInitialize(&planeContext,
            pSC->p0MBbuffer, pSC->p1MBbuffer, FALSE, i,
            &postProcessParameters, &highPassParameters);
        JxrInverseTransformFullResolutionPlaneApply(&planeContext, &geometry,
            pSC->m_param.bScaledArith, postProcessParameters.enabled,
            pSC->pPostProcInfo, mbX);
    }

    //================================================================
    // 420_UV
    for (i = 0; i < planePlan.chroma420ChannelCount && planePlan.transformsSamples; ++i)
    {
        JxrInverseTransformPlaneContext planeContext;
        JxrInverseTransformPlaneContextInitialize(&planeContext,
            pSC->p0MBbuffer, pSC->p1MBbuffer, TRUE, i,
            &postProcessParameters, &highPassParameters);
        JxrInverseTransformChroma420PlaneApply(&planeContext, &geometry,
            pSC->m_param.bScaledArith);
    }

    //================================================================
    // 422_UV
    for (i = 0; i < planePlan.chroma422ChannelCount && planePlan.transformsSamples; ++i)
    {
        JxrInverseTransformPlaneContext planeContext;
        JxrInverseTransformPlaneContextInitialize(&planeContext,
            pSC->p0MBbuffer, pSC->p1MBbuffer, TRUE, i,
            &postProcessParameters, &highPassParameters);
        JxrInverseTransformChroma422PlaneApply(&planeContext, &geometry,
            pSC->m_param.bScaledArith);
    }

    return ICERR_OK;
}

Int  invTransformMacroblock_alteredOperators_hard(CWMImageStrCodec * pSC)
{
    JxrInverseTransformMacroblockGeometry geometry;
    JxrInverseTransformPlanePlan planePlan;
    JxrHardTileBoundaryState hardTileState;
    JxrInverseTransformBoundaryContext boundaryContext;
    // const BITDEPTH_BITS bdBitDepth = pSC->WMII.bdBitDepth;
    size_t i;
    JxrInverseTransformMacroblockGeometryInitialize(&geometry,
        pSC->WMISCP.olOverlap, pSC->m_param.cfColorFormat,
        pSC->cColumn, pSC->cRow, pSC->cmbWidth, pSC->cmbHeight,
        pSC->m_param.cNumChannels, pSC->m_Dparam->cThumbnailScale);
    const OVERLAP olOverlap = geometry.overlap;
    const COLORFORMAT cfColorFormat = geometry.colorFormat;
    const Bool left = geometry.isLeft;
    const Bool top = geometry.isTop, bottom = geometry.isBottom;
    // Bool topAdjacentRow =  (pSC->cRow == 1), bottomAdjacentRow = (pSC->cRow == pSC->cmbHeight - 1);
    const size_t mbWidth = geometry.macroblockWidth;
    const size_t iChannels = geometry.channelCount;
    const size_t tScale = geometry.thumbnailScale;
    JxrInverseTransformPlanePlanInitialize(&planePlan,
        cfColorFormat, iChannels,
        tScale);

    JxrInversePostProcessParameters postProcessParameters;
    // ERR_CODE result = ICERR_OK;

    {
        JxrHardTileBoundaryConfiguration hardTileConfiguration;
        JxrHardTileBoundaryState previousHardTileState;

        hardTileConfiguration.enabled = pSC->WMISCP.bUseHardTileBoundaries;
        hardTileConfiguration.verticalSliceCountMinusOne = pSC->WMISCP.cNumOfSliceMinus1V;
        hardTileConfiguration.horizontalSliceCountMinusOne = pSC->WMISCP.cNumOfSliceMinus1H;
        hardTileConfiguration.verticalSliceColumns = pSC->WMISCP.uiTileY;
        hardTileConfiguration.horizontalSliceRows = pSC->WMISCP.uiTileX;
        previousHardTileState.tileX = pSC->tileX;
        previousHardTileState.tileY = pSC->tileY;
        previousHardTileState.previousMacroblockX = pSC->mbX;
        previousHardTileState.previousMacroblockY = pSC->mbY;
        previousHardTileState.isVerticalBoundary = pSC->bVertTileBoundary;
        previousHardTileState.isHorizontalBoundary = pSC->bHoriTileBoundary;
        previousHardTileState.isOneMacroblockLeftOfVerticalBoundary = pSC->bOneMBLeftVertTB;
        previousHardTileState.isOneMacroblockRightOfVerticalBoundary = pSC->bOneMBRightVertTB;
        JxrHardTileBoundaryStateCalculate(&hardTileState, &previousHardTileState,
            &hardTileConfiguration, pSC->cColumn, pSC->cRow);
        JxrInverseTransformBoundaryContextInitialize(&boundaryContext, &geometry, &hardTileState);
        pSC->tileX = hardTileState.tileX;
        pSC->tileY = hardTileState.tileY;
        pSC->mbX = hardTileState.previousMacroblockX;
        pSC->mbY = hardTileState.previousMacroblockY;
        pSC->bVertTileBoundary = hardTileState.isVerticalBoundary;
        pSC->bHoriTileBoundary = hardTileState.isHorizontalBoundary;
        pSC->bOneMBLeftVertTB = hardTileState.isOneMacroblockLeftOfVerticalBoundary;
        pSC->bOneMBRightVertTB = hardTileState.isOneMacroblockRightOfVerticalBoundary;
    }
    JxrInversePostProcessParametersInitialize(&postProcessParameters,
        pSC->WMII.cPostProcStrength, olOverlap, iChannels,
        pSC->pTile[pSC->cTileColumn].pQuantizerLP,
        pSC->pTile[pSC->cTileColumn].pQuantizerDC,
        pSC->MBInfo.iQIndexLP);
    if (postProcessParameters.enabled) {
        if (left) // a new MB row
            slideOneMBRow(pSC->pPostProcInfo, pSC->m_param.cNumChannels, mbWidth, top, bottom);  // previous current row becomes previous row
    }
    //================================================================
    // 400_Y, 444_YUV
    for (i = 0; i < planePlan.fullResolutionChannelCount && planePlan.transformsSamples; ++i)
    {
        JxrInverseTransformPlaneContext planeContext;
        JxrInverseTransformPlaneContextInitialize(&planeContext,
            pSC->p0MBbuffer, pSC->p1MBbuffer, FALSE, i,
            &postProcessParameters, NULL);
        JxrInverseTransformAlternateFullResolutionPlaneApply(&planeContext,
            &geometry, &boundaryContext, pSC->m_param.bScaledArith,
            postProcessParameters.enabled, pSC->pPostProcInfo,
            hardTileState.previousMacroblockX);
    }

    //================================================================
    // 420_UV
    for (i = 0; i < planePlan.chroma420ChannelCount && planePlan.transformsSamples; ++i)
    {
        JxrInverseTransformPlaneContext planeContext;
        JxrInverseTransformPlaneContextInitialize(&planeContext,
            pSC->p0MBbuffer, pSC->p1MBbuffer, TRUE, i,
            &postProcessParameters, NULL);
        JxrInverseTransformChroma420AlternatePlaneApply(&planeContext, &geometry,
            &boundaryContext, pSC->m_param.bScaledArith,
            pSC->iPredBefore[i], pSC->iPredAfter[i]);
    }

    //================================================================
    // 422_UV
    for (i = 0; i < planePlan.chroma422ChannelCount && planePlan.transformsSamples; ++i)
    {
        JxrInverseTransformPlaneContext planeContext;
        JxrInverseTransformPlaneContextInitialize(&planeContext,
            pSC->p0MBbuffer, pSC->p1MBbuffer, TRUE, i,
            &postProcessParameters, NULL);
        JxrInverseTransformChroma422AlternatePlaneApply(&planeContext, &geometry,
            &boundaryContext, pSC->m_param.bScaledArith,
            pSC->iPredBefore[i], pSC->iPredAfter[i]);
    }

    return ICERR_OK;
}
