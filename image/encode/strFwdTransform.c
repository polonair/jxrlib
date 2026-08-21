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
#include "JxrForwardTransformFullResolutionPlane.h"
#include "JxrForwardTransformChroma420Plane.h"
#include "JxrForwardTransformChroma422Plane.h"

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
    Int i;

    JxrForwardHardTileCodecStateAdapterUpdate(pSC, &hardTileState);
    JxrForwardTransformMacroblockGeometryInitialize(&geometry,
        pSC->WMISCP.olOverlap, pSC->m_param.cfColorFormat, pSC->cColumn, pSC->cRow,
        pSC->cmbWidth, pSC->cmbHeight, pSC->m_param.cNumChannels);
    JxrForwardTransformBoundaryContextInitialize(&boundaries, &geometry, &hardTileState);

    //================================================================
    // 400_Y, 444_YUV
    for(i = 0; i < (Int)geometry.fullResolutionPlaneCount; ++i)
    {
        JxrForwardTransformFullResolutionPlaneApply(pSC->p0MBbuffer[i], pSC->p1MBbuffer[i],
            i != 0, &geometry, &boundaries, pSC->m_param.bScaledArith);
    }

    //================================================================
    // 420_UV
    for(i = 0; i < (YUV_420 == geometry.colorFormat ? 2 : 0); ++i)
    {
        JxrForwardTransformChroma420PlaneApply(pSC->p0MBbuffer[1 + i], pSC->p1MBbuffer[1 + i],
            pSC->iPredBefore[i], pSC->iPredAfter[i], &geometry, &boundaries,
            pSC->m_param.bScaledArith);
    }

    //================================================================
    // 422_UV
    for(i = 0; i < (YUV_422 == geometry.colorFormat ? 2 : 0); ++i)
    {
        JxrForwardTransformChroma422PlaneApply(pSC->p0MBbuffer[1 + i], pSC->p1MBbuffer[1 + i],
            pSC->iPredBefore[i], pSC->iPredAfter[i], &geometry, &boundaries,
            pSC->m_param.bScaledArith);
    }
}

