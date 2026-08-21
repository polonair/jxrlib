#include "JxrForwardTransformChroma420Plane.h"

#include "encode.h"
#include "strTransform.h"

static Void JxrForwardTransformChroma420PlaneApplyFirstStageOverlap(
    PixelI* firstStage,
    PixelI* secondStage,
    const JxrForwardTransformMacroblockGeometry* geometry,
    const JxrForwardTransformBoundaryContext* boundaries)
{
    PixelI* samples;
    Int offset;

    if (geometry->overlap == OL_NONE)
        return;

    if (boundaries->hasTopBoundary && boundaries->hasLeftBoundary)
        strPre4(secondStage + 0, secondStage + 1, secondStage + 2, secondStage + 3);
    if (boundaries->hasTopBoundary && boundaries->hasRightBoundary)
        strPre4(secondStage - 27, secondStage - 28, secondStage - 25, secondStage - 26);
    if (boundaries->hasBottomBoundary && boundaries->hasLeftBoundary)
        strPre4(firstStage + 26, firstStage + 27, firstStage + 24, firstStage + 25);
    if (boundaries->hasBottomBoundary && boundaries->hasRightBoundary)
        strPre4(firstStage - 1, firstStage - 2, firstStage - 3, firstStage - 4);
    if (!geometry->isRight && !geometry->isBottom) {
        if (boundaries->hasTopBoundary) {
            for (offset = boundaries->hasLeftBoundary ? 0 : -32; offset < 32; offset += 32) {
                samples = secondStage + offset;
                strPre4(samples + 5, samples + 4, samples + 32, samples + 33);
                strPre4(samples + 7, samples + 6, samples + 34, samples + 35);
            }
        }
        else {
            for (offset = boundaries->hasLeftBoundary ? 0 : -32; offset < 32; offset += 32)
                strPre4x4Stage1Split(firstStage + 16 + offset, secondStage + offset, 32);
        }

        if (boundaries->hasLeftBoundary) {
            if (!geometry->isTop && !boundaries->isHorizontalTileBoundary) {
                strPre4(firstStage + 26, firstStage + 24, secondStage + 0, secondStage + 2);
                strPre4(firstStage + 27, firstStage + 25, secondStage + 1, secondStage + 3);
            }
            strPre4(secondStage + 10, secondStage + 8, secondStage + 16, secondStage + 18);
            strPre4(secondStage + 11, secondStage + 9, secondStage + 17, secondStage + 19);
        }
        else if (!boundaries->isVerticalTileBoundary) {
            strPre4x4Stage1(secondStage - 32, 32);
        }

        strPre4x4Stage1(secondStage, 32);
    }

    if (boundaries->hasBottomBoundary) {
        for (offset = boundaries->hasLeftBoundary ? 16 : -16;
            offset < (geometry->isRight ? -16 : 32); offset += 32) {
            samples = firstStage + offset;
            strPre4(samples + 15, samples + 14, samples + 42, samples + 43);
            strPre4(samples + 13, samples + 12, samples + 40, samples + 41);
        }
    }

    if (boundaries->hasRightBoundary && !geometry->isBottom) {
        if (!geometry->isTop && !boundaries->isHorizontalTileBoundary) {
            strPre4(firstStage - 1, firstStage - 3, secondStage - 27, secondStage - 25);
            strPre4(firstStage - 2, firstStage - 4, secondStage - 28, secondStage - 26);
        }
        strPre4(secondStage - 17, secondStage - 19, secondStage - 11, secondStage - 9);
        strPre4(secondStage - 18, secondStage - 20, secondStage - 12, secondStage - 10);
    }
}

static Void JxrForwardTransformChroma420PlaneApplyFirstStageTransform(
    PixelI* firstStage,
    PixelI* secondStage,
    const JxrForwardTransformMacroblockGeometry* geometry)
{
    Int offset;

    if (!geometry->isTop) {
        for (offset = geometry->isLeft ? 16 : -16;
            offset < (geometry->isRight ? 16 : 48); offset += 32)
            strDCT4x4Stage1(firstStage + offset);
    }
    if (!geometry->isBottom) {
        for (offset = geometry->isLeft ? 0 : -32;
            offset < (geometry->isRight ? 0 : 32); offset += 32)
            strDCT4x4Stage1(secondStage + offset);
    }
}

static Void JxrForwardTransformChroma420PlaneApplySecondStageOverlap(
    PixelI* firstStage,
    PixelI* secondStage,
    PixelI* predictionBefore,
    PixelI* predictionAfter,
    const JxrForwardTransformMacroblockGeometry* geometry,
    const JxrForwardTransformBoundaryContext* boundaries)
{
    if (geometry->overlap != OL_TWO)
        return;

    if (boundaries->isLeftAdjacentToVerticalBoundary && boundaries->hasTopBoundary)
        strTransformSubtractCornerPrediction(secondStage - 64, *(secondStage - 32));
    if (boundaries->isRightAdjacentToVerticalBoundary && boundaries->hasTopBoundary)
        predictionBefore[0] = *(secondStage + 0);
    if (boundaries->hasRightBoundary && boundaries->hasTopBoundary)
        strTransformSubtractCornerPrediction(secondStage - 32, predictionBefore[0]);

    if (boundaries->isLeftAdjacentToVerticalBoundary && boundaries->hasBottomBoundary)
        strTransformSubtractCornerPrediction(firstStage - 48, *(firstStage - 16));
    if (boundaries->isRightAdjacentToVerticalBoundary && boundaries->hasBottomBoundary)
        predictionBefore[1] = *(firstStage + 16);
    if (boundaries->hasRightBoundary && boundaries->hasBottomBoundary)
        strTransformSubtractCornerPrediction(firstStage - 16, predictionBefore[1]);

    if (boundaries->hasLeftOrRightBoundary && !boundaries->hasTopOrBottomBoundary) {
        if (boundaries->hasLeftBoundary)
            strPre2(firstStage + 16, secondStage);
        if (boundaries->hasRightBoundary)
            strPre2(firstStage - 16, secondStage - 32);
    }

    if (!geometry->isLeftOrRight) {
        if (boundaries->hasTopOrBottomBoundary && !boundaries->isVerticalTileBoundary) {
            if (boundaries->hasTopBoundary)
                strPre2(secondStage - 32, secondStage);
            if (boundaries->hasBottomBoundary)
                strPre2(firstStage - 16, firstStage + 16);
        }
        else if (!boundaries->hasTopOrBottomBoundary && !boundaries->isVerticalTileBoundary) {
            strPre2x2(firstStage - 16, firstStage + 16, secondStage - 32, secondStage);
        }
    }

    if (boundaries->isLeftAdjacentToVerticalBoundary && boundaries->hasTopBoundary)
        strTransformAddCornerPrediction(secondStage - 64, *(secondStage - 32));
    if (boundaries->isRightAdjacentToVerticalBoundary && boundaries->hasTopBoundary)
        predictionAfter[0] = *(secondStage + 0);
    if (boundaries->hasRightBoundary && boundaries->hasTopBoundary)
        strTransformAddCornerPrediction(secondStage - 32, predictionAfter[0]);

    if (boundaries->isLeftAdjacentToVerticalBoundary && boundaries->hasBottomBoundary)
        strTransformAddCornerPrediction(firstStage - 48, *(firstStage - 16));
    if (boundaries->isRightAdjacentToVerticalBoundary && boundaries->hasBottomBoundary)
        predictionAfter[1] = *(firstStage + 16);
    if (boundaries->hasRightBoundary && boundaries->hasBottomBoundary)
        strTransformAddCornerPrediction(firstStage - 16, predictionAfter[1]);
}

static Void JxrForwardTransformChroma420PlaneApplySecondStageTransform(
    PixelI* firstStage,
    const JxrForwardTransformMacroblockGeometry* geometry,
    Bool usesScaledArithmetic)
{
    if (geometry->isTopOrLeft)
        return;

    if (usesScaledArithmetic)
        strDCT2x2dnEnc(firstStage - 64, firstStage - 32, firstStage - 48, firstStage - 16);
    else
        strDCT2x2dn(firstStage - 64, firstStage - 32, firstStage - 48, firstStage - 16);
}

Void JxrForwardTransformChroma420PlaneApply(
    PixelI* firstStage,
    PixelI* secondStage,
    PixelI* predictionBefore,
    PixelI* predictionAfter,
    const JxrForwardTransformMacroblockGeometry* geometry,
    const JxrForwardTransformBoundaryContext* boundaries,
    Bool usesScaledArithmetic)
{
    JxrForwardTransformChroma420PlaneApplyFirstStageOverlap(
        firstStage, secondStage, geometry, boundaries);
    JxrForwardTransformChroma420PlaneApplyFirstStageTransform(
        firstStage, secondStage, geometry);
    JxrForwardTransformChroma420PlaneApplySecondStageOverlap(
        firstStage, secondStage, predictionBefore, predictionAfter, geometry, boundaries);
    JxrForwardTransformChroma420PlaneApplySecondStageTransform(
        firstStage, geometry, usesScaledArithmetic);
}
