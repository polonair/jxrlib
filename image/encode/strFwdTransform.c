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
#include "encode.h"
#include "JxrForwardTransformMath.h"
#include "JxrForwardTransformStages.h"
#include "JxrForwardHardTileCodecStateAdapter.h"
#include "JxrForwardTransformMacroblockGeometry.h"
#include "JxrForwardTransformBoundaryContext.h"

/** local functions **/
static Void strDCT2x2alt(PixelI * a, PixelI * b, PixelI * c, PixelI * d);

//static Void scaleDownUp0(PixelI *, PixelI *);
//static Void scaleDownUp1(PixelI *, PixelI *);
//static Void scaleDownUp2(PixelI *, PixelI *);
//#define FOURBUTTERFLY_ENC_ALT(p, i00, i01, i02, i03, i10, i11, i12, i13,	\
//    i20, i21, i22, i23, i30, i31, i32, i33)		\
//    JxrForwardTransformMathApplyHst4(&p[i00], &p[i01], &p[i02], &p[i03]);			\
//    JxrForwardTransformMathApplyHst4(&p[i10], &p[i11], &p[i12], &p[i13]);			\
//    JxrForwardTransformMathApplyHst4(&p[i20], &p[i21], &p[i22], &p[i23]);			\
//    JxrForwardTransformMathApplyHst4(&p[i30], &p[i31], &p[i32], &p[i33]);          \
//    JxrForwardTransformMathApplyHst1(&p[i00], &p[i03]);			\
//    JxrForwardTransformMathApplyHst1(&p[i10], &p[i13]);			\
//    JxrForwardTransformMathApplyHst1(&p[i20], &p[i23]);			\
//    JxrForwardTransformMathApplyHst1(&p[i30], &p[i33])

/** DCT stuff **/
/** data order before DCT **/
/**  0  1  2  3 **/
/**  4  5  6  7 **/
/**  8  9 10 11 **/
/** 12 13 14 15 **/
/** data order after DCT **/
/** 0  8  4  6 **/
/** 2 10 14 12 **/
/** 1 11 15 13 **/
/** 9  3  7  5 **/
/** reordering should be combined with zigzag scan **/

Void strDCT4x4Stage1(PixelI * p)
{
    JxrForwardTransformStagesApplyStage1Dct(p);
}

Void strDCT4x4SecondStage(PixelI * p)
{
    JxrForwardTransformStagesApplyStage2Dct(p);
}

Void strNormalizeEnc(PixelI* p, Bool bChroma)
{
    JxrForwardTransformMathNormalizeBlock(p, bChroma, 256, 16);
}

/** 2x2 DCT with pre-scaling - for use on encoder side **/
Void strDCT2x2dnEnc(PixelI *pa, PixelI *pb, PixelI *pc, PixelI *pd)
{
    JxrForwardTransformMathApplyDct2x2Down(pa, pb, pc, pd);
}

/** pre filter stuff **/
/** 2-point pre for boundaries **/
Void strPre2(PixelI * pa, PixelI * pb)
{
    JxrForwardTransformMathApplyPre2(pa, pb);
}

Void strPre2x2(PixelI *pa, PixelI *pb, PixelI *pc, PixelI *pd)
{
    JxrForwardTransformMathApplyPre2x2(pa, pb, pc, pd);
}

/** 4-point pre for boundaries **/
Void strPre4(PixelI *pa, PixelI *pb, PixelI *pc, PixelI *pd)
{
    JxrForwardTransformMathApplyPre4(pa, pb, pc, pd);
}

/*****************************************************************************************
  Input data offsets:
  (15)(14)|(10+64)(11+64) p0 (15)(14)|(74)(75)
  (13)(12)|( 8+64)( 9+64)    (13)(12)|(72)(73)
  --------+--------------    --------+--------
  ( 5)( 4)|( 0+64) (1+64) p1 ( 5)( 4)|(64)(65)
  ( 7)( 6)|( 2+64) (3+64)    ( 7)( 6)|(66)(67)
*****************************************************************************************/
Void strPre4x4Stage1Split(PixelI *p0, PixelI *p1, Int iOffset)
{
    JxrForwardTransformStagesApplyPreStage1Split(p0, p1, iOffset);
}

Void strPre4x4Stage1(PixelI* p, Int iOffset)
{
    strPre4x4Stage1Split(p, p + 16, iOffset);
}

/*****************************************************************************************
  Input data offsets:
  (15)(14)|(10+32)(11+32) p0 (15)(14)|(42)(43)
  (13)(12)|( 8+32)( 9+32)    (13)(12)|(40)(41)
  --------+--------------    --------+--------
  ( 5)( 4)|( 0+32)( 1+32) p1 ( 5)( 4)|(32)(33)
  ( 7)( 6)|( 2+32)( 3+32)    ( 7)( 6)|(34)(35)
*****************************************************************************************/
Void strPre4x4Stage2Split(PixelI* p0, PixelI* p1)
{
    JxrForwardTransformStagesApplyPreStage2Split(p0, p1);
}


/** 
    Hadamard+Scale transform
    for some strange reason, breaking up the function into two blocks, strHSTenc1 and strHSTenc
    seems to work faster
**/



/** Kron(Rotate(pi/8), Rotate(pi/8)) **/\
/** Kron(Rotate(pi/8), Rotate(pi/8)) **/

/** Kron(Rotate(pi/8), [1 1; 1 -1]/sqrt(2)) **/
/** [a b c d] => [D C A B] **/

/*************************************************************************
  Top-level function to tranform possible part of a macroblock
*************************************************************************/
Void transformMacroblock(CWMImageStrCodec * pSC)
{
    JxrForwardTransformMacroblockGeometry geometry;
    JxrForwardHardTileBoundaryState hardTileState;
    JxrForwardTransformBoundaryContext boundaries;
    PixelI * p = NULL;// * pt = NULL;
    Int i, j;

    JxrForwardHardTileCodecStateAdapterUpdate(pSC, &hardTileState);
    JxrForwardTransformMacroblockGeometryInitialize(&geometry,
        pSC->WMISCP.olOverlap, pSC->m_param.cfColorFormat, pSC->cColumn, pSC->cRow,
        pSC->cmbWidth, pSC->cmbHeight, pSC->m_param.cNumChannels);
    JxrForwardTransformBoundaryContextInitialize(&boundaries, &geometry, &hardTileState);

    //================================================================
    // 400_Y, 444_YUV
    for(i = 0; i < (Int)geometry.fullResolutionPlaneCount; ++i)
    {
        PixelI* const p0 = pSC->p0MBbuffer[i];//(0 == i ? pSC->pY0 : (1 == i ? pSC->pU0 : pSC->pV0));
        PixelI* const p1 = pSC->p1MBbuffer[i];//(0 == i ? pSC->pY1 : (1 == i ? pSC->pU1 : pSC->pV1));

        //================================
        // first level overlap
        if(OL_NONE != geometry.overlap)
        {
            /* Corner operations */
            if (boundaries.hasTopBoundary && boundaries.hasLeftBoundary)
                strPre4(p1 + 0, p1 + 1, p1 + 2, p1 + 3);
            if (boundaries.hasTopBoundary && boundaries.hasRightBoundary)
                strPre4(p1 - 59, p1 - 60, p1 - 57, p1 - 58);
            if (boundaries.hasBottomBoundary && boundaries.hasLeftBoundary)
                strPre4(p0 + 48 + 10, p0 + 48 + 11, p0 + 48 + 8, p0 + 48 + 9);
            if (boundaries.hasBottomBoundary && boundaries.hasRightBoundary)
                strPre4(p0 - 1, p0 - 2, p0 - 3, p0 - 4);
            if(!geometry.isRight && !geometry.isBottom)
            {
                if (boundaries.hasTopBoundary)
                {

                    for (j = (boundaries.hasLeftBoundary ? 0 : -64); j < 192; j += 64)
                    {
                        p = p1 + j;
                        strPre4(p + 5, p + 4, p + 64, p + 65);
                        strPre4(p + 7, p + 6, p + 66, p + 67);
                        p = NULL;
                    }
                }
                else
                {
                    for (j = (boundaries.hasLeftBoundary ? 0 : -64); j < 192; j += 64)
                    {
                        strPre4x4Stage1Split(p0 + 48 + j, p1 + j, 0);
                    }
                }

                if (boundaries.hasLeftBoundary)
                {
                    if (!geometry.isTop && !boundaries.isHorizontalTileBoundary)
                    {
                        strPre4(p0 + 58, p0 + 56, p1 + 0, p1 + 2);
                        strPre4(p0 + 59, p0 + 57, p1 + 1, p1 + 3);
                    }

                    for (j = -64; j < -16; j += 16)
                    {
                        p = p1 + j;
                        strPre4(p + 74, p + 72, p + 80, p + 82);
                        strPre4(p + 75, p + 73, p + 81, p + 83);
                        p = NULL;
                    }
                }
                else
                {
                    for (j = -64; j < -16; j += 16)
                    {
                        strPre4x4Stage1(p1 + j, 0);
                    }
                }

                strPre4x4Stage1(p1 +   0, 0);
                strPre4x4Stage1(p1 +  16, 0);
                strPre4x4Stage1(p1 +  32, 0);
                strPre4x4Stage1(p1 +  64, 0);
                strPre4x4Stage1(p1 +  80, 0);
                strPre4x4Stage1(p1 +  96, 0);
                strPre4x4Stage1(p1 + 128, 0);
                strPre4x4Stage1(p1 + 144, 0);
                strPre4x4Stage1(p1 + 160, 0);
            }
            
            if (boundaries.hasBottomBoundary)
            {
                for (j = (boundaries.hasLeftBoundary ? 48 : -16); j < (geometry.isRight ? -16 : 240); j += 64)
                {
                    p = p0 + j;
                    strPre4(p + 15, p + 14, p + 74, p + 75);
                    strPre4(p + 13, p + 12, p + 72, p + 73);
                    p = NULL;
                }
            }

            if (boundaries.hasRightBoundary && !geometry.isBottom)
            {
                if (!geometry.isTop && !boundaries.isHorizontalTileBoundary)
                {
                    strPre4(p0 - 1, p0 - 3, p1 - 59, p1 - 57);
                    strPre4(p0 - 2, p0 - 4, p1 - 60, p1 - 58);
                }
                for (j = -64; j < -16; j += 16)
                {
                    p = p1 + j;
                    strPre4(p + 15, p + 13, p + 21, p + 23);
                    strPre4(p + 14, p + 12, p + 20, p + 22);
                    p = NULL;
                }
            }
        }

        //================================
        // first level transform
        if (!geometry.isTop)
        {
            for (j = (geometry.isLeft ? 48 : -16); j < (geometry.isRight ? 48 : 240); j += 64)
            {
                strDCT4x4Stage1(p0 + j);
            }
        }

        if (!geometry.isBottom)
        {
            for (j = (geometry.isLeft ? 0 : -64); j < (geometry.isRight ? 0 : 192); j += 64)
            {
                strDCT4x4Stage1(p1 + j + 0);
                strDCT4x4Stage1(p1 + j + 16);
                strDCT4x4Stage1(p1 + j + 32);
            }
        }
        
        //================================
        // second level overlap
        if (OL_TWO == geometry.overlap)
        {
            /* Corner operations */
            if (boundaries.hasTopBoundary && boundaries.hasLeftBoundary)
                strPre4(p1 + 0, p1 + 64, p1 + 0 + 16, p1 + 64 + 16);
            if (boundaries.hasTopBoundary && boundaries.hasRightBoundary)
                strPre4(p1 - 128, p1 - 64, p1 - 128 + 16, p1 - 64 + 16); 
            if (boundaries.hasBottomBoundary && boundaries.hasLeftBoundary)
                strPre4(p0 + 32, p0 + 96, p0 + 32 + 16, p0 + 96 + 16);
            if (boundaries.hasBottomBoundary && boundaries.hasRightBoundary)
                strPre4(p0 - 96, p0 - 32, p0 - 96 + 16, p0 - 32 + 16);
            if (boundaries.hasLeftOrRightBoundary && (!boundaries.hasTopOrBottomBoundary))
            {
                if (boundaries.hasLeftBoundary) {
                    j = 0;
                    strPre4(p0 + j + 32, p0 + j +  48, p1 + j +  0, p1 + j + 16);
                    strPre4(p0 + j + 96, p0 + j + 112, p1 + j + 64, p1 + j + 80);
                }
                if (boundaries.hasRightBoundary) {
                    j = -128;
                    strPre4(p0 + j + 32, p0 + j +  48, p1 + j +  0, p1 + j + 16);
                    strPre4(p0 + j + 96, p0 + j + 112, p1 + j + 64, p1 + j + 80);
                }
            }

            if (!boundaries.hasLeftOrRightBoundary)
            {
                if (boundaries.hasTopOrBottomBoundary)
                {
                    if (boundaries.hasTopBoundary) {
                        p = p1;
                        strPre4(p - 128, p - 64, p +  0, p + 64);
                        strPre4(p - 112, p - 48, p + 16, p + 80);
                        p = NULL;
                    }
                    if (boundaries.hasBottomBoundary) {
                        p = p0 + 32;
                        strPre4(p - 128, p - 64, p +  0, p + 64);
                        strPre4(p - 112, p - 48, p + 16, p + 80);
                        p = NULL;
                    }
                }
                else
                {
                    strPre4x4Stage2Split(p0, p1);
                }
            }
        }

        //================================
        // second level transform
        if (!geometry.isTopOrLeft){
            if (pSC->m_param.bScaledArith) {
                strNormalizeEnc(p0 - 256, (i != 0));
            }
            strDCT4x4SecondStage(p0 - 256);
        }
    }

    //================================================================
    // 420_UV
    for(i = 0; i < (YUV_420 == geometry.colorFormat? 2 : 0); ++i)
    {
        PixelI* const p0 = pSC->p0MBbuffer[1 + i];//(0 == i ? pSC->pU0 : pSC->pV0);
        PixelI* const p1 = pSC->p1MBbuffer[1 + i];//(0 == i ? pSC->pU1 : pSC->pV1);

        //================================
        // first level overlap (420_UV)
        if (OL_NONE != geometry.overlap)
        {
            /* Corner operations */
            if (boundaries.hasTopBoundary && boundaries.hasLeftBoundary)
                strPre4(p1 + 0, p1 + 1, p1 + 2, p1 + 3);
            if (boundaries.hasTopBoundary && boundaries.hasRightBoundary)
                strPre4(p1 - 27, p1 - 28, p1 - 25, p1 - 26);
            if (boundaries.hasBottomBoundary && boundaries.hasLeftBoundary)
                strPre4(p0 + 16 + 10, p0 + 16 + 11, p0 + 16 + 8, p0 + 16 + 9);
            if (boundaries.hasBottomBoundary && boundaries.hasRightBoundary)
                strPre4(p0 - 1, p0 - 2, p0 - 3, p0 - 4);
            if(!geometry.isRight && !geometry.isBottom)
            {
                if (boundaries.hasTopBoundary)
                {

                    for (j = (boundaries.hasLeftBoundary ? 0 : -32); j < 32; j += 32)
                    {
                        p = p1 + j;
                        strPre4(p + 5, p + 4, p + 32, p + 33);
                        strPre4(p + 7, p + 6, p + 34, p + 35);
                        p = NULL;
                    }
                }
                else
                {
                    for (j = (boundaries.hasLeftBoundary ? 0: -32); j < 32; j += 32)
                    {
                        strPre4x4Stage1Split(p0 + 16 + j, p1 + j, 32);
                    }
                }

                if (boundaries.hasLeftBoundary)
                {
                    if (!geometry.isTop && !boundaries.isHorizontalTileBoundary)
                    {
                        strPre4(p0 + 26, p0 + 24, p1 + 0, p1 + 2);
                        strPre4(p0 + 27, p0 + 25, p1 + 1, p1 + 3);
                    }

                    strPre4(p1 + 10, p1 + 8, p1 + 16, p1 + 18);
                    strPre4(p1 + 11, p1 + 9, p1 + 17, p1 + 19);
                }
                else if (!boundaries.isVerticalTileBoundary)
                {
                    strPre4x4Stage1(p1 - 32, 32);
                }

                strPre4x4Stage1(p1, 32);
            }

            if (boundaries.hasBottomBoundary)
            {
                for (j = (boundaries.hasLeftBoundary ? 16: -16); j < (geometry.isRight ? -16: 32); j += 32)
                {
                    p = p0 + j;
                    strPre4(p + 15, p + 14, p + 42, p + 43);
                    strPre4(p + 13, p + 12, p + 40, p + 41);
                    p = NULL;
                }
            }

            if (boundaries.hasRightBoundary && !geometry.isBottom)
            {
                if (!geometry.isTop && !boundaries.isHorizontalTileBoundary)
                {
                    strPre4(p0 - 1, p0 - 3, p1 - 27, p1 - 25);
                    strPre4(p0 - 2, p0 - 4, p1 - 28, p1 - 26);
                }

                strPre4(p1 - 17, p1 - 19, p1 - 11, p1 -  9);
                strPre4(p1 - 18, p1 - 20, p1 - 12, p1 - 10);
            }
        }    

        //================================
        // first level transform (420_UV)
        if (!geometry.isTop)
        {
            for (j = (geometry.isLeft ? 16 : -16); j < (geometry.isRight ? 16 : 48); j += 32)
            {
                strDCT4x4Stage1(p0 + j);
            }
        }

        if (!geometry.isBottom)
        {
            for (j = (geometry.isLeft ? 0 : -32); j < (geometry.isRight ? 0 : 32); j += 32)
            {
                strDCT4x4Stage1(p1 + j);
            }
        }
        
        //================================
        // second level overlap (420_UV)
        if (OL_TWO == geometry.overlap)
        {
            if (boundaries.isLeftAdjacentToVerticalBoundary && boundaries.hasTopBoundary)
                strTransformSubtractCornerPrediction(p1 - 64 + 0, *(p1 - 64 + 32));

            if (boundaries.isRightAdjacentToVerticalBoundary && boundaries.hasTopBoundary)
                pSC->iPredBefore[i][0] = *(p1 + 0);
            if (boundaries.hasRightBoundary && boundaries.hasTopBoundary)
                strTransformSubtractCornerPrediction(p1 - 64 + 32, pSC->iPredBefore[i][0]);

            if (boundaries.isLeftAdjacentToVerticalBoundary && boundaries.hasBottomBoundary)
                strTransformSubtractCornerPrediction(p0 - 64 + 16, *(p0 - 64 + 48));

            if (boundaries.isRightAdjacentToVerticalBoundary && boundaries.hasBottomBoundary)
                pSC->iPredBefore[i][1] = *(p0 + 16);
            if (boundaries.hasRightBoundary && boundaries.hasBottomBoundary)
                strTransformSubtractCornerPrediction(p0 - 64 + 48, pSC->iPredBefore[i][1]);

            if (boundaries.hasLeftOrRightBoundary && !boundaries.hasTopOrBottomBoundary)
            {
                if (boundaries.hasLeftBoundary)
                    strPre2(p0 + 0 + 16, p1 + 0);
                if (boundaries.hasRightBoundary)
                    strPre2(p0 + -32 + 16, p1 + -32);
            }

            if (!geometry.isLeftOrRight)
            {
                if (boundaries.hasTopOrBottomBoundary && !boundaries.isVerticalTileBoundary)
                {
                    if (boundaries.hasTopBoundary)
                        strPre2(p1 - 32, p1);
                    if (boundaries.hasBottomBoundary)
                        strPre2(p0 + 16 - 32, p0 + 16);
                }
                else if (!boundaries.hasTopOrBottomBoundary && !boundaries.isVerticalTileBoundary)
                    strPre2x2(p0 - 16, p0 + 16, p1 - 32, p1);
            }
            if (boundaries.isLeftAdjacentToVerticalBoundary && boundaries.hasTopBoundary)
                strTransformAddCornerPrediction(p1 - 64 + 0, *(p1 - 64 + 32));
            if (boundaries.isRightAdjacentToVerticalBoundary && boundaries.hasTopBoundary)
                pSC->iPredAfter[i][0] = *(p1 + 0);
            if (boundaries.hasRightBoundary && boundaries.hasTopBoundary)
                strTransformAddCornerPrediction(p1 - 64 + 32, pSC->iPredAfter[i][0]);
            if (boundaries.isLeftAdjacentToVerticalBoundary && boundaries.hasBottomBoundary)
                strTransformAddCornerPrediction(p0 - 64 + 16, *(p0 - 64 + 48));
            if (boundaries.isRightAdjacentToVerticalBoundary && boundaries.hasBottomBoundary)
                pSC->iPredAfter[i][1] = *(p0 + 16);
            if (boundaries.hasRightBoundary && boundaries.hasBottomBoundary)
                strTransformAddCornerPrediction(p0 - 64 + 48, pSC->iPredAfter[i][1]);
        }

        //================================
        // second level transform (420_UV)
        if (!geometry.isTopOrLeft)
        {
            if (!pSC->m_param.bScaledArith) {
                strDCT2x2dn(p0 - 64, p0 - 32, p0 - 48, p0 - 16);
            }
            else {
                strDCT2x2dnEnc(p0 - 64, p0 - 32, p0 - 48, p0 - 16);
            }
        }
    }

    //================================================================
    //  422_UV
    for(i = 0; i < (YUV_422 == geometry.colorFormat? 2 : 0); ++i)
    {
        PixelI* const p0 = pSC->p0MBbuffer[1 + i];//(0 == i ? pSC->pU0 : pSC->pV0);
        PixelI* const p1 = pSC->p1MBbuffer[1 + i];//(0 == i ? pSC->pU1 : pSC->pV1);

        //================================
        // first level overlap (422_UV)
        if (OL_NONE != geometry.overlap)
        {
            /* Corner operations */
            if (boundaries.hasTopBoundary && boundaries.hasLeftBoundary)
                strPre4(p1 + 0, p1 + 1, p1 + 2, p1 + 3);
            if (boundaries.hasTopBoundary && boundaries.hasRightBoundary)
                strPre4(p1 - 59, p1 - 60, p1 - 57, p1 - 58);
            if (boundaries.hasBottomBoundary && boundaries.hasLeftBoundary)
                strPre4(p0 + 48 + 10, p0 + 48 + 11, p0 + 48 + 8, p0 + 48 + 9);
            if (boundaries.hasBottomBoundary && boundaries.hasRightBoundary)
                strPre4(p0 - 1, p0 - 2, p0 - 3, p0 - 4);
            if(!geometry.isRight && !geometry.isBottom)
            {
                if (boundaries.hasTopBoundary)
                {

                    for (j = (boundaries.hasLeftBoundary ? 0 : -64); j < 64; j += 64)
                    {
                        p = p1 + j;
                        strPre4(p + 5, p + 4, p + 64, p + 65);
                        strPre4(p + 7, p + 6, p + 66, p + 67);
                        p = NULL;
                    }
                }
                else
                {
                    for (j = (boundaries.hasLeftBoundary ? 0: -64); j < 64; j += 64)
                    {
                        strPre4x4Stage1Split(p0 + 48 + j, p1 + j, 0);
                    }
                }

                if (boundaries.hasLeftBoundary)
                {
                    if (!geometry.isTop && !boundaries.isHorizontalTileBoundary)
                    {
                        strPre4(p0 + 58, p0 + 56, p1 + 0, p1 + 2);
                        strPre4(p0 + 59, p0 + 57, p1 + 1, p1 + 3);
                    }

                    for (j = 0; j < 48; j += 16)
                    {
                        p = p1 + j;
                        strPre4(p + 10, p + 8, p + 16, p + 18);
                        strPre4(p + 11, p + 9, p + 17, p + 19);
                        p = NULL;
                    }
                }
                else if (!boundaries.isVerticalTileBoundary)
                {
                    for (j = -64; j < -16; j += 16)
                    {
                        strPre4x4Stage1(p1 + j, 0);
                    }
                }

                strPre4x4Stage1(p1 +  0, 0);
                strPre4x4Stage1(p1 + 16, 0);
                strPre4x4Stage1(p1 + 32, 0);
            }

            if (boundaries.hasBottomBoundary)
            {
                for (j = (boundaries.hasLeftBoundary ? 48: -16); j < (geometry.isRight ? -16: 112); j += 64)
                {
                    p = p0 + j;
                    strPre4(p + 15, p + 14, p + 74, p + 75);
                    strPre4(p + 13, p + 12, p + 72, p + 73);
                    p = NULL;
                }
            }

            if (boundaries.hasRightBoundary && !geometry.isBottom)
            {
                if (!geometry.isTop && !boundaries.isHorizontalTileBoundary)
                {
                    strPre4(p0 - 1, p0 - 3, p1 - 59, p1 - 57);
                    strPre4(p0 - 2, p0 - 4, p1 - 60, p1 - 58);
                }

                for (j = -64; j < -16; j += 16)
                {
                    p = p1 + j;
                    strPre4(p + 15, p + 13, p + 21, p + 23);
                    strPre4(p + 14, p + 12, p + 20, p + 22);
                    p = NULL;
                }
            }
        }    

        //================================
        // first level transform (422_UV)
        if (!geometry.isTop)
        {
            for (j = (geometry.isLeft ? 48 : -16); j < (geometry.isRight ? 48 : 112); j += 64)
            {
                strDCT4x4Stage1(p0 + j);
            }
        }

        if (!geometry.isBottom)
        {
            for (j = (geometry.isLeft ? 0 : -64); j < (geometry.isRight ? 0 : 64); j += 64)
            {
                strDCT4x4Stage1(p1 + j + 0);
                strDCT4x4Stage1(p1 + j + 16);
                strDCT4x4Stage1(p1 + j + 32);
            }
        }
        
        //================================
        // second level overlap (422_UV)
        if (OL_TWO == geometry.overlap)
        {
            if (boundaries.isLeftAdjacentToVerticalBoundary && boundaries.hasTopBoundary)
                strTransformSubtractCornerPrediction(p1 - 128 + 0, *(p1 - 128 + 64));

            if (boundaries.isRightAdjacentToVerticalBoundary && boundaries.hasTopBoundary)
                pSC->iPredBefore[i][0] = *(p1 + 0);
            if (boundaries.hasRightBoundary && boundaries.hasTopBoundary)
                strTransformSubtractCornerPrediction(p1 - 128 + 64, pSC->iPredBefore[i][0]);

            if (boundaries.isLeftAdjacentToVerticalBoundary && boundaries.hasBottomBoundary)
                strTransformSubtractCornerPrediction(p0 - 128 + 48, *(p0 - 128 + 112));

            if (boundaries.isRightAdjacentToVerticalBoundary && boundaries.hasBottomBoundary)
                pSC->iPredBefore[i][1] = *(p0 + 48);
            if (boundaries.hasRightBoundary && boundaries.hasBottomBoundary)
                strTransformSubtractCornerPrediction(p0 - 128 + 112, pSC->iPredBefore[i][1]);

            if (!geometry.isBottom)
            {
                if (boundaries.hasLeftOrRightBoundary)
                {
                    if (!geometry.isTop && !boundaries.isHorizontalTileBoundary)
                    {
                        if (boundaries.hasLeftBoundary)
                            strPre2(p0 + 48 + 0, p1 + 0);

                        if (boundaries.hasRightBoundary)
                            strPre2(p0 + 48 + -64, p1 + -64);
                    }

                    if (boundaries.hasLeftBoundary)
                        strPre2(p1 + 16, p1 + 16 + 16);

                    if (boundaries.hasRightBoundary)
                        strPre2(p1 + -48, p1 + -48 + 16);
                }

                if (!boundaries.hasLeftOrRightBoundary)
                {
                    if (boundaries.hasTopBoundary)
                        strPre2(p1 - 64, p1);
                    else
                        strPre2x2(p0 - 16, p0 + 48, p1 - 64, p1);

                    strPre2x2(p1 - 48, p1 + 16, p1 - 32, p1 + 32);
                }
            }

            if (boundaries.hasBottomBoundary && (!boundaries.hasLeftOrRightBoundary))
                strPre2(p0 - 16, p0 + 48);

            if (boundaries.isLeftAdjacentToVerticalBoundary && boundaries.hasTopBoundary)
                strTransformAddCornerPrediction(p1 - 128 + 0, *(p1 - 128 + 64));

            if (boundaries.isRightAdjacentToVerticalBoundary && boundaries.hasTopBoundary)
                pSC->iPredAfter[i][0] = *(p1 + 0);
            if (boundaries.hasRightBoundary && boundaries.hasTopBoundary)
                strTransformAddCornerPrediction(p1 - 128 + 64, pSC->iPredAfter[i][0]);

            if (boundaries.isLeftAdjacentToVerticalBoundary && boundaries.hasBottomBoundary)
                strTransformAddCornerPrediction(p0 - 128 + 48, *(p0 - 128 + 112));

            if (boundaries.isRightAdjacentToVerticalBoundary && boundaries.hasBottomBoundary)
                pSC->iPredAfter[i][1] = *(p0 + 48);
            if (boundaries.hasRightBoundary && boundaries.hasBottomBoundary)
                strTransformAddCornerPrediction(p0 - 128 + 112, pSC->iPredAfter[i][1]);
        }

        //================================
        // second level transform (422_UV)
        if (!geometry.isTopOrLeft)
        {
            if (!pSC->m_param.bScaledArith) {
                strDCT2x2dn(p0 - 128, p0 - 64, p0 - 112, p0 - 48);
                strDCT2x2dn(p0 -  96, p0 - 32, p0 -  80, p0 - 16);
            }
            else {
                strDCT2x2dnEnc(p0 - 128, p0 - 64, p0 - 112, p0 - 48);
                strDCT2x2dnEnc(p0 -  96, p0 - 32, p0 -  80, p0 - 16);
            }

            // 1D lossless HT
            p0[- 96] -= p0[-128];
            p0[-128] += ((p0[-96] + 1) >> 1);
        }
    }
    assert(NULL == p);
}

