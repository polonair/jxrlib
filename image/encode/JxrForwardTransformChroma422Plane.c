#include "JxrForwardTransformChroma422Plane.h"

#include "encode.h"
#include "strTransform.h"

static Void JxrForwardTransformChroma422PlaneApplyFirstStageOverlap(
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
        strPre4(secondStage - 59, secondStage - 60, secondStage - 57, secondStage - 58);
    if (boundaries->hasBottomBoundary && boundaries->hasLeftBoundary)
        strPre4(firstStage + 58, firstStage + 59, firstStage + 56, firstStage + 57);
    if (boundaries->hasBottomBoundary && boundaries->hasRightBoundary)
        strPre4(firstStage - 1, firstStage - 2, firstStage - 3, firstStage - 4);
    if (!geometry->isRight && !geometry->isBottom) {
        if (boundaries->hasTopBoundary) {
            for (offset = boundaries->hasLeftBoundary ? 0 : -64; offset < 64; offset += 64) {
                samples = secondStage + offset;
                strPre4(samples + 5, samples + 4, samples + 64, samples + 65);
                strPre4(samples + 7, samples + 6, samples + 66, samples + 67);
            }
        }
        else {
            for (offset = boundaries->hasLeftBoundary ? 0 : -64; offset < 64; offset += 64)
                strPre4x4Stage1Split(firstStage + 48 + offset, secondStage + offset, 0);
        }

        if (boundaries->hasLeftBoundary) {
            if (!geometry->isTop && !boundaries->isHorizontalTileBoundary) {
                strPre4(firstStage + 58, firstStage + 56, secondStage + 0, secondStage + 2);
                strPre4(firstStage + 59, firstStage + 57, secondStage + 1, secondStage + 3);
            }
            for (offset = 0; offset < 48; offset += 16) {
                samples = secondStage + offset;
                strPre4(samples + 10, samples + 8, samples + 16, samples + 18);
                strPre4(samples + 11, samples + 9, samples + 17, samples + 19);
            }
        }
        else if (!boundaries->isVerticalTileBoundary) {
            for (offset = -64; offset < -16; offset += 16)
                strPre4x4Stage1(secondStage + offset, 0);
        }

        strPre4x4Stage1(secondStage + 0, 0);
        strPre4x4Stage1(secondStage + 16, 0);
        strPre4x4Stage1(secondStage + 32, 0);
    }

    if (boundaries->hasBottomBoundary) {
        for (offset = boundaries->hasLeftBoundary ? 48 : -16;
            offset < (geometry->isRight ? -16 : 112); offset += 64) {
            samples = firstStage + offset;
            strPre4(samples + 15, samples + 14, samples + 74, samples + 75);
            strPre4(samples + 13, samples + 12, samples + 72, samples + 73);
        }
    }

    if (boundaries->hasRightBoundary && !geometry->isBottom) {
        if (!geometry->isTop && !boundaries->isHorizontalTileBoundary) {
            strPre4(firstStage - 1, firstStage - 3, secondStage - 59, secondStage - 57);
            strPre4(firstStage - 2, firstStage - 4, secondStage - 60, secondStage - 58);
        }
        for (offset = -64; offset < -16; offset += 16) {
            samples = secondStage + offset;
            strPre4(samples + 15, samples + 13, samples + 21, samples + 23);
            strPre4(samples + 14, samples + 12, samples + 20, samples + 22);
        }
    }
}

static Void JxrForwardTransformChroma422PlaneApplyFirstStageTransform(
    PixelI* firstStage,
    PixelI* secondStage,
    const JxrForwardTransformMacroblockGeometry* geometry)
{
    Int offset;

    if (!geometry->isTop) {
        for (offset = geometry->isLeft ? 48 : -16;
            offset < (geometry->isRight ? 48 : 112); offset += 64)
            strDCT4x4Stage1(firstStage + offset);
    }
    if (!geometry->isBottom) {
        for (offset = geometry->isLeft ? 0 : -64;
            offset < (geometry->isRight ? 0 : 64); offset += 64) {
            strDCT4x4Stage1(secondStage + offset + 0);
            strDCT4x4Stage1(secondStage + offset + 16);
            strDCT4x4Stage1(secondStage + offset + 32);
        }
    }
}

static Void JxrForwardTransformChroma422PlaneRemovePredictions(
    PixelI* firstStage,
    PixelI* secondStage,
    PixelI* predictionBefore,
    const JxrForwardTransformBoundaryContext* boundaries)
{
    if (boundaries->isLeftAdjacentToVerticalBoundary && boundaries->hasTopBoundary)
        strTransformSubtractCornerPrediction(secondStage - 128, *(secondStage - 64));
    if (boundaries->isRightAdjacentToVerticalBoundary && boundaries->hasTopBoundary)
        predictionBefore[0] = *(secondStage + 0);
    if (boundaries->hasRightBoundary && boundaries->hasTopBoundary)
        strTransformSubtractCornerPrediction(secondStage - 64, predictionBefore[0]);
    if (boundaries->isLeftAdjacentToVerticalBoundary && boundaries->hasBottomBoundary)
        strTransformSubtractCornerPrediction(firstStage - 80, *(firstStage - 16));
    if (boundaries->isRightAdjacentToVerticalBoundary && boundaries->hasBottomBoundary)
        predictionBefore[1] = *(firstStage + 48);
    if (boundaries->hasRightBoundary && boundaries->hasBottomBoundary)
        strTransformSubtractCornerPrediction(firstStage - 16, predictionBefore[1]);
}

static Void JxrForwardTransformChroma422PlaneRestorePredictions(
    PixelI* firstStage,
    PixelI* secondStage,
    PixelI* predictionAfter,
    const JxrForwardTransformBoundaryContext* boundaries)
{
    if (boundaries->isLeftAdjacentToVerticalBoundary && boundaries->hasTopBoundary)
        strTransformAddCornerPrediction(secondStage - 128, *(secondStage - 64));
    if (boundaries->isRightAdjacentToVerticalBoundary && boundaries->hasTopBoundary)
        predictionAfter[0] = *(secondStage + 0);
    if (boundaries->hasRightBoundary && boundaries->hasTopBoundary)
        strTransformAddCornerPrediction(secondStage - 64, predictionAfter[0]);
    if (boundaries->isLeftAdjacentToVerticalBoundary && boundaries->hasBottomBoundary)
        strTransformAddCornerPrediction(firstStage - 80, *(firstStage - 16));
    if (boundaries->isRightAdjacentToVerticalBoundary && boundaries->hasBottomBoundary)
        predictionAfter[1] = *(firstStage + 48);
    if (boundaries->hasRightBoundary && boundaries->hasBottomBoundary)
        strTransformAddCornerPrediction(firstStage - 16, predictionAfter[1]);
}

static Void JxrForwardTransformChroma422PlaneApplySecondStageOverlap(
    PixelI* firstStage,
    PixelI* secondStage,
    PixelI* predictionBefore,
    PixelI* predictionAfter,
    const JxrForwardTransformMacroblockGeometry* geometry,
    const JxrForwardTransformBoundaryContext* boundaries)
{
    if (geometry->overlap != OL_TWO)
        return;

    JxrForwardTransformChroma422PlaneRemovePredictions(
        firstStage, secondStage, predictionBefore, boundaries);

    if (!geometry->isBottom) {
        if (boundaries->hasLeftOrRightBoundary) {
            if (!geometry->isTop && !boundaries->isHorizontalTileBoundary) {
                if (boundaries->hasLeftBoundary)
                    strPre2(firstStage + 48, secondStage);
                if (boundaries->hasRightBoundary)
                    strPre2(firstStage - 16, secondStage - 64);
            }
            if (boundaries->hasLeftBoundary)
                strPre2(secondStage + 16, secondStage + 32);
            if (boundaries->hasRightBoundary)
                strPre2(secondStage - 48, secondStage - 32);
        }

        if (!boundaries->hasLeftOrRightBoundary) {
            if (boundaries->hasTopBoundary)
                strPre2(secondStage - 64, secondStage);
            else
                strPre2x2(firstStage - 16, firstStage + 48, secondStage - 64, secondStage);
            strPre2x2(secondStage - 48, secondStage + 16, secondStage - 32, secondStage + 32);
        }
    }

    if (boundaries->hasBottomBoundary && !boundaries->hasLeftOrRightBoundary)
        strPre2(firstStage - 16, firstStage + 48);

    JxrForwardTransformChroma422PlaneRestorePredictions(
        firstStage, secondStage, predictionAfter, boundaries);
}

static Void JxrForwardTransformChroma422PlaneApplySecondStageTransform(
    PixelI* firstStage,
    const JxrForwardTransformMacroblockGeometry* geometry,
    Bool usesScaledArithmetic)
{
    if (geometry->isTopOrLeft)
        return;

    if (usesScaledArithmetic) {
        strDCT2x2dnEnc(firstStage - 128, firstStage - 64, firstStage - 112, firstStage - 48);
        strDCT2x2dnEnc(firstStage - 96, firstStage - 32, firstStage - 80, firstStage - 16);
    }
    else {
        strDCT2x2dn(firstStage - 128, firstStage - 64, firstStage - 112, firstStage - 48);
        strDCT2x2dn(firstStage - 96, firstStage - 32, firstStage - 80, firstStage - 16);
    }

    firstStage[-96] -= firstStage[-128];
    firstStage[-128] += ((firstStage[-96] + 1) >> 1);
}

Void JxrForwardTransformChroma422PlaneApply(
    PixelI* firstStage,
    PixelI* secondStage,
    PixelI* predictionBefore,
    PixelI* predictionAfter,
    const JxrForwardTransformMacroblockGeometry* geometry,
    const JxrForwardTransformBoundaryContext* boundaries,
    Bool usesScaledArithmetic)
{
    JxrForwardTransformChroma422PlaneApplyFirstStageOverlap(
        firstStage, secondStage, geometry, boundaries);
    JxrForwardTransformChroma422PlaneApplyFirstStageTransform(
        firstStage, secondStage, geometry);
    JxrForwardTransformChroma422PlaneApplySecondStageOverlap(
        firstStage, secondStage, predictionBefore, predictionAfter, geometry, boundaries);
    JxrForwardTransformChroma422PlaneApplySecondStageTransform(
        firstStage, geometry, usesScaledArithmetic);
}
