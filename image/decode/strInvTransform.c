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
#include "JxrHardTileCodecStateAdapter.h"
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
#include "JxrInverseTransformNormalMacroblock.h"
#include "JxrInverseTransformNormalCodecSetup.h"
#include "JxrInverseTransformAlternateMacroblock.h"
#include "JxrInverseTransformAlternateCodecSetup.h"
#include "JxrInverseTransformCodecInvocation.h"
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
    JxrInverseTransformNormalCodecSetup setup;
    JxrInverseTransformCodecInvocation invocation;

    JxrInverseTransformNormalCodecSetupInitialize(&setup, pSC);
    JxrInverseTransformCodecInvocationInitialize(&invocation, pSC);
    JxrInverseTransformCodecInvocationAdvancePostProcessRow(&invocation,
        &setup.geometry, setup.postProcessParameters.enabled);

    {
        JxrInverseTransformNormalMacroblock macroblock;
        macroblock.geometry = &setup.geometry;
        macroblock.planePlan = &setup.planePlan;
        macroblock.postProcessParameters = &setup.postProcessParameters;
        macroblock.highPassParameters = &setup.highPassParameters;
        macroblock.firstStagePlanes = invocation.firstStagePlanes;
        macroblock.secondStagePlanes = invocation.secondStagePlanes;
        memcpy(macroblock.postProcessInfo, invocation.postProcessInfo, sizeof(macroblock.postProcessInfo));
        macroblock.macroblockColumn = setup.geometry.macroblockColumn;
        macroblock.usesScaledArithmetic = invocation.usesScaledArithmetic;
        JxrInverseTransformNormalMacroblockProcess(&macroblock);
    }

    return ICERR_OK;
}
Int  invTransformMacroblock_alteredOperators_hard(CWMImageStrCodec * pSC)
{
    JxrInverseTransformAlternateCodecSetup setup;
    JxrInverseTransformCodecInvocation invocation;

    JxrInverseTransformAlternateCodecSetupInitialize(&setup, pSC);
    JxrInverseTransformCodecInvocationInitialize(&invocation, pSC);
    JxrInverseTransformCodecInvocationAdvancePostProcessRow(&invocation,
        &setup.geometry, setup.postProcessParameters.enabled);

    {
        JxrInverseTransformAlternateMacroblock macroblock;
        macroblock.geometry = &setup.geometry;
        macroblock.planePlan = &setup.planePlan;
        macroblock.boundaries = &setup.boundaryContext;
        macroblock.postProcessParameters = &setup.postProcessParameters;
        macroblock.firstStagePlanes = invocation.firstStagePlanes;
        macroblock.secondStagePlanes = invocation.secondStagePlanes;
        memcpy(macroblock.postProcessInfo, invocation.postProcessInfo, sizeof(macroblock.postProcessInfo));
        macroblock.predictionBefore = invocation.predictionBefore;
        macroblock.predictionAfter = invocation.predictionAfter;
        macroblock.macroblockColumn = setup.hardTileState.previousMacroblockX;
        macroblock.usesScaledArithmetic = invocation.usesScaledArithmetic;
        JxrInverseTransformAlternateMacroblockProcess(&macroblock);
    }

    return ICERR_OK;
}
