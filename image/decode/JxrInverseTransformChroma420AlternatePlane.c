#include "JxrInverseTransformChroma420AlternatePlane.h"

#include "JxrInverseTransformMath.h"
#include "JxrTransformMath.h"
#include "strTransform.h"

extern Void strIDCT4x4Stage1(PixelI* samples);
extern Void strPost4x4Stage1_alternate(PixelI* samples, Int offset);
extern Void strPost4x4Stage1Split_alternate(PixelI* first, PixelI* second, Int offset);

static Void JxrInverseTransformChroma420AlternatePlaneApplySecondStageTransform(
    PixelI* secondStage,
    const JxrInverseTransformMacroblockGeometry* geometry,
    Bool usesScaledArithmetic)
{
    if (geometry->isBottomOrRight)
        return;

    if (usesScaledArithmetic)
        JxrInverseTransformMathApplyScaledDct2x2Down(secondStage, secondStage + 32,
            secondStage + 16, secondStage + 48);
    else
        JxrTransformMathApplyDct2x2Down(secondStage, secondStage + 32,
            secondStage + 16, secondStage + 48);
}

static Void JxrInverseTransformChroma420AlternatePlaneRemoveCornerPredictions(
    PixelI* firstStage,
    PixelI* secondStage,
    const JxrInverseTransformBoundaryContext* boundaries,
    PixelI* predictionBefore)
{
    if (boundaries->isLeftAdjacentToVerticalBoundary && boundaries->hasTopBoundary)
        JxrInverseTransformMathSubtractCornerPredictionAt(secondStage, -64, secondStage[-32]);
    if (boundaries->isRightAdjacentToVerticalBoundary && boundaries->hasTopBoundary)
        predictionBefore[0] = secondStage[0];
    if (boundaries->hasRightBoundary && boundaries->hasTopBoundary)
        JxrInverseTransformMathSubtractCornerPredictionAt(secondStage, -32, predictionBefore[0]);
    if (boundaries->isLeftAdjacentToVerticalBoundary && boundaries->hasBottomBoundary)
        JxrInverseTransformMathSubtractCornerPredictionAt(firstStage, -48, firstStage[-16]);
    if (boundaries->isRightAdjacentToVerticalBoundary && boundaries->hasBottomBoundary)
        predictionBefore[1] = firstStage[16];
    if (boundaries->hasRightBoundary && boundaries->hasBottomBoundary)
        JxrInverseTransformMathSubtractCornerPredictionAt(firstStage, -16, predictionBefore[1]);
}

static Void JxrInverseTransformChroma420AlternatePlaneRestoreCornerPredictions(
    PixelI* firstStage,
    PixelI* secondStage,
    const JxrInverseTransformBoundaryContext* boundaries,
    PixelI* predictionAfter)
{
    if (boundaries->isLeftAdjacentToVerticalBoundary && boundaries->hasTopBoundary)
        JxrInverseTransformMathAddCornerPredictionAt(secondStage, -64, secondStage[-32]);
    if (boundaries->isRightAdjacentToVerticalBoundary && boundaries->hasTopBoundary)
        predictionAfter[0] = secondStage[0];
    if (boundaries->hasRightBoundary && boundaries->hasTopBoundary)
        JxrInverseTransformMathAddCornerPredictionAt(secondStage, -32, predictionAfter[0]);
    if (boundaries->isLeftAdjacentToVerticalBoundary && boundaries->hasBottomBoundary)
        JxrInverseTransformMathAddCornerPredictionAt(firstStage, -48, firstStage[-16]);
    if (boundaries->isRightAdjacentToVerticalBoundary && boundaries->hasBottomBoundary)
        predictionAfter[1] = firstStage[16];
    if (boundaries->hasRightBoundary && boundaries->hasBottomBoundary)
        JxrInverseTransformMathAddCornerPredictionAt(firstStage, -16, predictionAfter[1]);
}

static Void JxrInverseTransformChroma420AlternatePlaneApplySecondStageOverlap(
    PixelI* firstStage,
    PixelI* secondStage,
    const JxrInverseTransformMacroblockGeometry* geometry,
    const JxrInverseTransformBoundaryContext* boundaries,
    PixelI* predictionBefore,
    PixelI* predictionAfter)
{
    if (geometry->overlap != OL_TWO)
        return;

    JxrInverseTransformChroma420AlternatePlaneRemoveCornerPredictions(
        firstStage, secondStage, boundaries, predictionBefore);

    if (boundaries->hasLeftOrRightBoundary && !boundaries->hasTopOrBottomBoundary) {
        if (boundaries->hasLeftBoundary)
            JxrInverseTransformMathApplyAlternatePost2(firstStage + 16, secondStage);
        if (boundaries->hasRightBoundary)
            JxrInverseTransformMathApplyAlternatePost2(firstStage - 16, secondStage - 32);
    }
    if (!geometry->isLeftOrRight) {
        if (boundaries->hasTopOrBottomBoundary && !boundaries->isVerticalTileBoundary) {
            if (boundaries->hasTopBoundary)
                JxrInverseTransformMathApplyAlternatePost2(secondStage - 32, secondStage);
            if (boundaries->hasBottomBoundary)
                JxrInverseTransformMathApplyAlternatePost2(firstStage - 16, firstStage + 16);
        }
        else if (!boundaries->hasTopOrBottomBoundary && !boundaries->isVerticalTileBoundary) {
            JxrInverseTransformMathApplyAlternatePost2x2(firstStage - 16, firstStage + 16,
                secondStage - 32, secondStage);
        }
    }

    JxrInverseTransformChroma420AlternatePlaneRestoreCornerPredictions(
        firstStage, secondStage, boundaries, predictionAfter);
}

static Void JxrInverseTransformChroma420AlternatePlaneApplyFirstStageTransform(
    PixelI* firstStage,
    PixelI* secondStage,
    const JxrInverseTransformMacroblockGeometry* geometry,
    const JxrInverseTransformBoundaryContext* boundaries)
{
    Int offset;

    if (!geometry->isTop) {
        for (offset = geometry->isLeft ? 48 :
                (boundaries->isLeftAdjacentToVerticalBoundary ? -48 : -16);
            offset < (boundaries->hasRightBoundary ? 16 : 48); offset += 32)
            strIDCT4x4Stage1(firstStage + offset);
    }
    if (!geometry->isBottom) {
        for (offset = geometry->isLeft ? 32 :
                (boundaries->isLeftAdjacentToVerticalBoundary ? -64 : -32);
            offset < (boundaries->hasRightBoundary ? 0 : 32); offset += 32)
            strIDCT4x4Stage1(secondStage + offset);
    }
}

static Void JxrInverseTransformChroma420AlternatePlaneApplyFirstStageOverlap(
    PixelI* firstStage,
    PixelI* secondStage,
    const JxrInverseTransformMacroblockGeometry* geometry,
    const JxrInverseTransformBoundaryContext* boundaries)
{
    PixelI* edge;

    if (geometry->overlap == OL_NONE)
        return;

    if (boundaries->hasTopBoundary && boundaries->isLeftAdjacentToVerticalBoundary)
        JxrInverseTransformMathApplyAlternatePost4(secondStage - 64, secondStage - 63,
            secondStage - 62, secondStage - 61);
    if (boundaries->hasTopBoundary && boundaries->hasRightBoundary)
        JxrInverseTransformMathApplyAlternatePost4(secondStage - 27, secondStage - 28,
            secondStage - 25, secondStage - 26);
    if (boundaries->hasBottomBoundary && boundaries->isLeftAdjacentToVerticalBoundary)
        JxrInverseTransformMathApplyAlternatePost4(firstStage - 38, firstStage - 37,
            firstStage - 40, firstStage - 39);
    if (boundaries->hasBottomBoundary && boundaries->hasRightBoundary)
        JxrInverseTransformMathApplyAlternatePost4(firstStage - 1, firstStage - 2,
            firstStage - 3, firstStage - 4);

    if (!geometry->isLeft && !geometry->isTop) {
        if (boundaries->isLeftAdjacentToVerticalBoundary) {
            if (!geometry->isBottom && !boundaries->isHorizontalTileBoundary) {
                JxrInverseTransformMathApplyAlternatePost4(firstStage - 38, firstStage - 40,
                    secondStage - 64, secondStage - 62);
                JxrInverseTransformMathApplyAlternatePost4(firstStage - 37, firstStage - 39,
                    secondStage - 63, secondStage - 61);
            }
            JxrInverseTransformMathApplyAlternatePost4(firstStage - 54, firstStage - 56,
                firstStage - 48, firstStage - 46);
            JxrInverseTransformMathApplyAlternatePost4(firstStage - 53, firstStage - 55,
                firstStage - 47, firstStage - 45);
        }

        if (boundaries->hasBottomBoundary) {
            edge = firstStage - 48;
            JxrInverseTransformMathApplyAlternatePost4(edge + 15, edge + 14, edge + 42, edge + 43);
            JxrInverseTransformMathApplyAlternatePost4(edge + 13, edge + 12, edge + 40, edge + 41);
            if (!geometry->isRight && !boundaries->isVerticalTileBoundary) {
                edge = firstStage - 16;
                JxrInverseTransformMathApplyAlternatePost4(edge + 15, edge + 14, edge + 42, edge + 43);
                JxrInverseTransformMathApplyAlternatePost4(edge + 13, edge + 12, edge + 40, edge + 41);
            }
        }
        else {
            strPost4x4Stage1Split_alternate(firstStage - 48, secondStage - 64, 32);
            if (!geometry->isRight && !boundaries->isVerticalTileBoundary)
                strPost4x4Stage1Split_alternate(firstStage - 16, secondStage - 32, 32);
        }

        if (boundaries->hasRightBoundary) {
            if (!geometry->isBottom && !boundaries->isHorizontalTileBoundary) {
                JxrInverseTransformMathApplyAlternatePost4(firstStage - 2, firstStage - 4,
                    secondStage - 28, secondStage - 26);
                JxrInverseTransformMathApplyAlternatePost4(firstStage - 1, firstStage - 3,
                    secondStage - 27, secondStage - 25);
            }
            JxrInverseTransformMathApplyAlternatePost4(firstStage - 18, firstStage - 20,
                firstStage - 12, firstStage - 10);
            JxrInverseTransformMathApplyAlternatePost4(firstStage - 17, firstStage - 19,
                firstStage - 11, firstStage - 9);
        }
        else {
            strPost4x4Stage1_alternate(firstStage - 32, 32);
        }
        strPost4x4Stage1_alternate(firstStage - 64, 32);
    }

    if (boundaries->hasTopBoundary) {
        if (!geometry->isLeft) {
            edge = secondStage - 60;
            JxrInverseTransformMathApplyAlternatePost4(edge + 1, edge, edge + 28, edge + 29);
            JxrInverseTransformMathApplyAlternatePost4(edge + 3, edge + 2, edge + 30, edge + 31);
        }
        if (!geometry->isLeft && !geometry->isRight && !boundaries->isVerticalTileBoundary) {
            edge = secondStage - 28;
            JxrInverseTransformMathApplyAlternatePost4(edge + 1, edge, edge + 28, edge + 29);
            JxrInverseTransformMathApplyAlternatePost4(edge + 3, edge + 2, edge + 30, edge + 31);
        }
    }
}

Void JxrInverseTransformChroma420AlternatePlaneApply(
    const JxrInverseTransformPlaneContext* plane,
    const JxrInverseTransformMacroblockGeometry* geometry,
    const JxrInverseTransformBoundaryContext* boundaries,
    Bool usesScaledArithmetic,
    PixelI* predictionBefore,
    PixelI* predictionAfter)
{
    PixelI* firstStage = plane->buffers.firstStage;
    PixelI* secondStage = plane->buffers.secondStage;

    JxrInverseTransformChroma420AlternatePlaneApplySecondStageTransform(
        secondStage, geometry, usesScaledArithmetic);
    JxrInverseTransformChroma420AlternatePlaneApplySecondStageOverlap(firstStage, secondStage,
        geometry, boundaries, predictionBefore, predictionAfter);
    if (geometry->thumbnailScale >= 4)
        return;
    JxrInverseTransformChroma420AlternatePlaneApplyFirstStageTransform(
        firstStage, secondStage, geometry, boundaries);
    JxrInverseTransformChroma420AlternatePlaneApplyFirstStageOverlap(
        firstStage, secondStage, geometry, boundaries);
}
