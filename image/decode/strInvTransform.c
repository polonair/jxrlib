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
#include "JxrInverseTransformMacroblockGeometry.h"
#include "JxrHardTileBoundaryState.h"
#include "JxrInverseTransformBoundaryContext.h"
#include "JxrInversePostProcessParameters.h"
#include "JxrInverseHighPassParameters.h"
#include "JxrInverseTransformPlanePlan.h"
#include "JxrInverseTransformPlaneBuffers.h"
#include "JxrInverseTransformPlaneContext.h"
#include "JxrInverseTransformPlaneStage2.h"
#include "JxrInverseTransformPlaneStage2Alternate.h"
#include "JxrInverseTransformPlaneStage1Normal.h"
#include "JxrInverseTransformPlaneStage1Alternate.h"
#include "JxrInverseTransformChroma420Plane.h"
#include "JxrInverseTransformChroma422Plane.h"
#include "JxrInverseTransformChroma420AlternatePlane.h"
#include "JxrInverseTransformChroma422AlternatePlane.h"
#include "JxrTransformMath.h"
static const Int JxrInverseTransformStage2P0FirstOffsets[4] = { -96, -32, -80, -16 };
static const Int JxrInverseTransformStage2P0SecondOffsets[4] = { 96, 32, 112, 48 };
static const Int JxrInverseTransformStage2P1FirstOffsets[4] = { -112, -48, -128, -64 };
static const Int JxrInverseTransformStage2P1SecondOffsets[4] = { 80, 16, 64, 0 };

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
    /** top left corner, butterfly => butterfly **/
    JxrTransformMathApplyDct2x2Up(p + 0, p + 1, p + 2, p + 3);

    /** top right corner, -pi/8 rotation => butterfly **/
    JxrInverseTransformMathApplyOdd(p + 5, p + 4, p + 7, p + 6);

    /** bottom left corner, butterfly => -pi/8 rotation **/
    JxrInverseTransformMathApplyOdd(p + 10, p + 8, p + 11, p + 9);

    /** bottom right corner, -pi/8 rotation => -pi/8 rotation **/
    JxrInverseTransformMathApplyOddOdd(p + 15, p + 14, p + 13, p + 12);
    
    /** butterfly **/
    //FOURBUTTERFLY(p, 0, 4, 8, 12, 1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15);
    JxrTransformMathApplyFourButterfly(p, JxrTransformFirstStageFourButterflyOffsets);
}

Void strIDCT4x4Stage2(PixelI* p)
{
    /** bottom left corner, butterfly => -pi/8 rotation **/
    JxrInverseTransformMathApplyOdd(p + 32, p + 48, p + 96, p + 112);
    
    /** top right corner, -pi/8 rotation => butterfly **/
    JxrInverseTransformMathApplyOdd(p + 128, p + 192, p + 144, p + 208);
    
    /** bottom right corner, -pi/8 rotation => -pi/8 rotation **/
    JxrInverseTransformMathApplyOddOdd(p + 160, p + 224, p + 176, p + 240);

    /** top left corner, butterfly => butterfly **/
    JxrTransformMathApplyDct2x2Up(p + 0, p + 64, p + 16, p + 80);
    
    /** butterfly **/
    JxrTransformMathApplyFourButterfly(p, JxrTransformSecondStageFourButterflyOffsets);
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
typedef enum JxrInverseTransformOverlapMode {
    JxrInverseTransformOverlapNormal,
    JxrInverseTransformOverlapAlternate
} JxrInverseTransformOverlapMode;

static Void JxrInverseTransformApplyStage1Split(
    PixelI *p0,
    PixelI *p1,
    Int iOffset,
    Int iHPQP,
    Bool bHPAbsent,
    JxrInverseTransformOverlapMode mode)
{
    Int column;
    Int directCurrent[4];
    Int temporaryCurrent;
    PixelI *p2 = p0 + 72 - iOffset;
    PixelI *p3 = p1 + 64 - iOffset;

    p0 += 12;
    p1 += 4;

    /* Apply the 2x2 DCT to each of the four aligned columns. */
    for (column = 0; column < 4; ++column) {
        JxrTransformMathApplyDct2x2Down(p0 + column, p2 + column, p1 + column, p3 + column);
    }

    /* Transform the bottom-right corner as one 4-point operation. */
    JxrInverseTransformMathApplyOddOddPost(p3 + 0, p3 + 1, p3 + 2, p3 + 3);

    /* Rotate the two anti-diagonal corners. */
    JxrInverseTransformMathRotateHalf(&p1[2], &p1[3]);
    JxrInverseTransformMathRotateHalf(&p1[0], &p1[1]);
    JxrInverseTransformMathRotateHalf(&p2[1], &p2[3]);
    JxrInverseTransformMathRotateHalf(&p2[0], &p2[2]);

    /* The first Hadamard+scale pass is the only transform difference by mode. */
    for (column = 0; column < 4; ++column) {
        if (mode == JxrInverseTransformOverlapAlternate) {
            JxrInverseTransformMathApplyAlternateHadamardScale2(p0 + column, p3 + column);
        }
        else {
            JxrInverseTransformMathApplyHadamardScale2(p0 + column, p3 + column);
        }
    }

    /* The second Hadamard+scale pass is shared by both modes. */
    for (column = 0; column < 4; ++column) {
        JxrInverseTransformMathApplyHadamardScale4(p0 + column, p2 + column, p1 + column, p3 + column);
    }

    if (mode == JxrInverseTransformOverlapAlternate) {
        return;
    }

    /* Compute all direct-current values before any compensation changes samples. */
    for (column = 0; column < 4; ++column) {
        temporaryCurrent = (p0[column] + p1[column] + p2[column] + p3[column]) >> 1;
        directCurrent[column] = (temporaryCurrent * 595 + 65536) >> 17;
    }

    /* Apply the optional direct-current compensation to each column. */
    for (column = 0; column < 4; ++column) {
        JxrInverseTransformMathApplyConditionalDcCompensation(
            p0 + column, p2 + column, p1 + column, p3 + column,
            directCurrent[column], iHPQP, bHPAbsent);
    }
}

Void strPost4x4Stage1Split(PixelI *p0, PixelI *p1, Int iOffset, Int iHPQP, Bool bHPAbsent)
{
    JxrInverseTransformApplyStage1Split(
        p0, p1, iOffset, iHPQP, bHPAbsent, JxrInverseTransformOverlapNormal);
}

Void strPost4x4Stage1(PixelI* p, Int iOffset, Int iHPQP, Bool bHPAbsent)
{
    strPost4x4Stage1Split(p, p + 16, iOffset, iHPQP, bHPAbsent);
}

Void strPost4x4Stage1Split_alternate(PixelI *p0, PixelI *p1, Int iOffset)
{
    JxrInverseTransformApplyStage1Split(
        p0, p1, iOffset, 0, FALSE, JxrInverseTransformOverlapAlternate);
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
static Void JxrInverseTransformApplyStage2Split(
    PixelI* p0,
    PixelI* p1,
    JxrInverseTransformOverlapMode mode)
{
    Int column;

    /* Apply the 2x2 DCT to each logical column in its legacy scan order. */
    for (column = 0; column < 4; ++column) {
        JxrTransformMathApplyDct2x2Down(
            p0 + JxrInverseTransformStage2P0FirstOffsets[column], p0 + JxrInverseTransformStage2P0SecondOffsets[column],
            p1 + JxrInverseTransformStage2P1FirstOffsets[column], p1 + JxrInverseTransformStage2P1SecondOffsets[column]);
    }

    /* Transform the bottom-right corner as one 4-point operation. */
    JxrInverseTransformMathApplyOddOddPost(p1 + 0, p1 + 64, p1 + 16, p1 + 80);

    /* Rotate the two anti-diagonal corners. */
    JxrInverseTransformMathRotateHalf(&p0[48], &p0[32]);
    JxrInverseTransformMathRotateHalf(&p0[112], &p0[96]);
    JxrInverseTransformMathRotateHalf(&p1[-64], &p1[-128]);
    JxrInverseTransformMathRotateHalf(&p1[-48], &p1[-112]);

    /* The first Hadamard+scale pass is the only transform difference by mode. */
    for (column = 0; column < 4; ++column) {
        if (mode == JxrInverseTransformOverlapAlternate) {
            JxrInverseTransformMathApplyAlternateHadamardScale2(
                p0 + JxrInverseTransformStage2P0FirstOffsets[column],
                p1 + JxrInverseTransformStage2P1SecondOffsets[column]);
        }
        else {
            JxrInverseTransformMathApplyHadamardScale2(
                p0 + JxrInverseTransformStage2P0FirstOffsets[column],
                p1 + JxrInverseTransformStage2P1SecondOffsets[column]);
        }
    }

    /* The second Hadamard+scale pass is shared by both modes. */
    for (column = 0; column < 4; ++column) {
        JxrInverseTransformMathApplyHadamardScale4(
            p0 + JxrInverseTransformStage2P0FirstOffsets[column], p1 + JxrInverseTransformStage2P1FirstOffsets[column],
            p0 + JxrInverseTransformStage2P0SecondOffsets[column], p1 + JxrInverseTransformStage2P1SecondOffsets[column]);
    }
}

Void strPost4x4Stage2Split(PixelI* p0, PixelI* p1)
{
    JxrInverseTransformApplyStage2Split(p0, p1, JxrInverseTransformOverlapNormal);
}

Void strPost4x4Stage2Split_alternate(PixelI* p0, PixelI* p1)
{
    JxrInverseTransformApplyStage2Split(p0, p1, JxrInverseTransformOverlapAlternate);
}
/*************************************************************************
  Top-level function to inverse tranform possible part of a macroblock
*************************************************************************/
Int  invTransformMacroblock(CWMImageStrCodec * pSC)
{
    JxrInverseTransformMacroblockGeometry geometry;
    JxrInverseTransformPlanePlan planePlan;
    // const BITDEPTH_BITS bdBitDepth = pSC->WMII.bdBitDepth;
    PixelI * p = NULL;// * pt = NULL;
    size_t i;
    JxrInverseTransformMacroblockGeometryInitialize(&geometry,
        pSC->WMISCP.olOverlap, pSC->m_param.cfColorFormat,
        pSC->cColumn, pSC->cRow, pSC->cmbWidth, pSC->cmbHeight,
        pSC->m_param.cNumChannels, pSC->m_Dparam->cThumbnailScale);
    const OVERLAP olOverlap = geometry.overlap;
    const COLORFORMAT cfColorFormat = geometry.colorFormat;
    const Bool left = geometry.isLeft, right = geometry.isRight;
    const Bool top = geometry.isTop, bottom = geometry.isBottom;
    const Bool topORbottom = geometry.isTopOrBottom, leftORright = geometry.isLeftOrRight;
    const Bool topORleft = geometry.isTopOrLeft, bottomORright = geometry.isBottomOrRight;
    const size_t mbWidth = geometry.macroblockWidth, mbX = geometry.macroblockColumn;
    const size_t iChannels = geometry.channelCount;
    const size_t tScale = geometry.thumbnailScale;
    JxrInverseTransformPlanePlanInitialize(&planePlan,
        cfColorFormat, iChannels,
        tScale);
    Int j = 0;

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
        PixelI* const p0 = planeContext.buffers.firstStage;
        PixelI* const p1 = planeContext.buffers.secondStage;





        //================================
        // second level inverse transform
        if (!bottomORright)
        {
            if(postProcessParameters.enabled)
                updatePostProcInfo(pSC->pPostProcInfo, p1, mbX, i); // update postproc info before IDCT

            JxrInverseTransformPlaneStage2Apply(p1, (i != 0), pSC->m_param.bScaledArith);

        }

        //================================
        // second level inverse overlap
        if (OL_TWO == olOverlap)
        {
            if (leftORright && (!topORbottom))
            {
                j = left ? 0 : -128;
                JxrInverseTransformMathApplyPost4(p0 + j + 32, p0 + j +  48, p1 + j +  0, p1 + j + 16);
                JxrInverseTransformMathApplyPost4(p0 + j + 96, p0 + j + 112, p1 + j + 64, p1 + j + 80);
            }

            if (!leftORright)
            {
                if (topORbottom)
                {
                    p = top ? p1 : p0 + 32;
                    JxrInverseTransformMathApplyPost4(p - 128, p - 64, p +  0, p + 64);
                    JxrInverseTransformMathApplyPost4(p - 112, p - 48, p + 16, p + 80);
                    p = NULL;
                }
                else
                {
                    strPost4x4Stage2Split(p0, p1);
                }
            }
        }

        if(postProcessParameters.enabled)
            postProcMB(pSC->pPostProcInfo, p0, p1, mbX, i, planeContext.directCurrentQuantizer); // second stage deblocking

        //================================
        JxrInverseTransformPlaneStage1NormalApply(p0, p1, olOverlap,
            left, right, top, bottom, topORbottom, leftORright,
            planeContext.highPassQuantizer, planeContext.isHighPassAbsent, tScale);
        if(postProcessParameters.enabled && (!topORleft))
            postProcBlock(pSC->pPostProcInfo, p0, p1, mbX, i, planeContext.lowPassQuantizer); // destairing and first stage deblocking
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
    const Bool left = geometry.isLeft, right = geometry.isRight;
    const Bool top = geometry.isTop, bottom = geometry.isBottom;
    const Bool leftORright = geometry.isLeftOrRight;
    const Bool topORleft = geometry.isTopOrLeft, bottomORright = geometry.isBottomOrRight;
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
        PixelI* const p0 = planeContext.buffers.firstStage;
        PixelI* const p1 = planeContext.buffers.secondStage;


        //================================
        // second level inverse transform
        if (!bottomORright)
        {
            if(postProcessParameters.enabled)
                updatePostProcInfo(pSC->pPostProcInfo, p1, hardTileState.previousMacroblockX, i); // update postproc info before IDCT

            JxrInverseTransformPlaneStage2Apply(p1, (i != 0), pSC->m_param.bScaledArith);

        }

        //================================
        // second level inverse overlap
        JxrInverseTransformPlaneStage2AlternateApply(p0, p1, olOverlap,
            leftORright, &boundaryContext);

        if(postProcessParameters.enabled)
            postProcMB(pSC->pPostProcInfo, p0, p1, hardTileState.previousMacroblockX, i, planeContext.directCurrentQuantizer); // second stage deblocking

        //================================
        JxrInverseTransformPlaneStage1AlternateApply(p0, p1, olOverlap,
            left, right, top, bottom, &boundaryContext, tScale);
        if(postProcessParameters.enabled && (!topORleft))
            postProcBlock(pSC->pPostProcInfo, p0, p1, hardTileState.previousMacroblockX, i, planeContext.lowPassQuantizer); // destairing and first stage deblocking
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
