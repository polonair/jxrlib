#include "JxrInverseTransformChroma422AlternatePlane.h"

#include "JxrInverseTransformMath.h"
#include "JxrTransformMath.h"
#include "strTransform.h"

extern Void strIDCT4x4Stage1(PixelI* samples);
extern Void strPost4x4Stage1_alternate(PixelI* samples, Int offset);
extern Void strPost4x4Stage1Split_alternate(PixelI* first, PixelI* second, Int offset);

static Void JxrInverseTransformChroma422AlternatePlaneApplySecondStageTransform(PixelI* secondStage,
    const JxrInverseTransformMacroblockGeometry* geometry, Bool usesScaledArithmetic)
{
    if (geometry->isBottomOrRight || geometry->thumbnailScale >= 16) return;
    secondStage[0] -= (secondStage[32] + 1) >> 1;
    secondStage[32] += secondStage[0];
    if (usesScaledArithmetic) {
        JxrInverseTransformMathApplyScaledDct2x2Down(secondStage, secondStage + 64, secondStage + 16, secondStage + 80);
        JxrInverseTransformMathApplyScaledDct2x2Down(secondStage + 32, secondStage + 96, secondStage + 48, secondStage + 112);
    } else {
        JxrTransformMathApplyDct2x2Down(secondStage, secondStage + 64, secondStage + 16, secondStage + 80);
        JxrTransformMathApplyDct2x2Down(secondStage + 32, secondStage + 96, secondStage + 48, secondStage + 112);
    }
}

static Void JxrInverseTransformChroma422AlternatePlaneRemovePredictions(PixelI* firstStage, PixelI* secondStage,
    const JxrInverseTransformBoundaryContext* boundaries, PixelI* before)
{
    if (boundaries->isLeftAdjacentToVerticalBoundary && boundaries->hasTopBoundary) JxrInverseTransformMathSubtractCornerPredictionAt(secondStage, -128, secondStage[-64]);
    if (boundaries->isRightAdjacentToVerticalBoundary && boundaries->hasTopBoundary) before[0] = secondStage[0];
    if (boundaries->hasRightBoundary && boundaries->hasTopBoundary) JxrInverseTransformMathSubtractCornerPredictionAt(secondStage, -64, before[0]);
    if (boundaries->isLeftAdjacentToVerticalBoundary && boundaries->hasBottomBoundary) JxrInverseTransformMathSubtractCornerPredictionAt(firstStage, -80, firstStage[-16]);
    if (boundaries->isRightAdjacentToVerticalBoundary && boundaries->hasBottomBoundary) before[1] = firstStage[48];
    if (boundaries->hasRightBoundary && boundaries->hasBottomBoundary) JxrInverseTransformMathSubtractCornerPredictionAt(firstStage, -16, before[1]);
}

static Void JxrInverseTransformChroma422AlternatePlaneRestorePredictions(PixelI* firstStage, PixelI* secondStage,
    const JxrInverseTransformBoundaryContext* boundaries, PixelI* after)
{
    if (boundaries->isLeftAdjacentToVerticalBoundary && boundaries->hasTopBoundary) JxrInverseTransformMathAddCornerPredictionAt(secondStage, -128, secondStage[-64]);
    if (boundaries->isRightAdjacentToVerticalBoundary && boundaries->hasTopBoundary) after[0] = secondStage[0];
    if (boundaries->hasRightBoundary && boundaries->hasTopBoundary) JxrInverseTransformMathAddCornerPredictionAt(secondStage, -64, after[0]);
    if (boundaries->isLeftAdjacentToVerticalBoundary && boundaries->hasBottomBoundary) JxrInverseTransformMathAddCornerPredictionAt(firstStage, -80, firstStage[-16]);
    if (boundaries->isRightAdjacentToVerticalBoundary && boundaries->hasBottomBoundary) after[1] = firstStage[48];
    if (boundaries->hasRightBoundary && boundaries->hasBottomBoundary) JxrInverseTransformMathAddCornerPredictionAt(firstStage, -16, after[1]);
}

static Void JxrInverseTransformChroma422AlternatePlaneApplySecondStageOverlap(PixelI* firstStage, PixelI* secondStage,
    const JxrInverseTransformMacroblockGeometry* geometry, const JxrInverseTransformBoundaryContext* boundaries,
    PixelI* before, PixelI* after)
{
    if (geometry->overlap != OL_TWO) return;
    JxrInverseTransformChroma422AlternatePlaneRemovePredictions(firstStage, secondStage, boundaries, before);
    if (!geometry->isBottom) {
        if (boundaries->hasLeftOrRightBoundary) {
            if (!geometry->isTop && !boundaries->isHorizontalTileBoundary) {
                if (boundaries->hasLeftBoundary) JxrInverseTransformMathApplyAlternatePost2(firstStage + 48, secondStage);
                if (boundaries->hasRightBoundary) JxrInverseTransformMathApplyAlternatePost2(firstStage - 16, secondStage - 64);
            }
            if (boundaries->hasLeftBoundary) JxrInverseTransformMathApplyAlternatePost2(secondStage + 16, secondStage + 32);
            if (boundaries->hasRightBoundary) JxrInverseTransformMathApplyAlternatePost2(secondStage - 48, secondStage - 32);
        }
        if (!boundaries->hasLeftOrRightBoundary) {
            if (boundaries->hasTopBoundary) JxrInverseTransformMathApplyAlternatePost2(secondStage - 64, secondStage);
            else JxrInverseTransformMathApplyAlternatePost2x2(firstStage - 16, firstStage + 48, secondStage - 64, secondStage);
            JxrInverseTransformMathApplyAlternatePost2x2(secondStage - 48, secondStage + 16, secondStage - 32, secondStage + 32);
        }
    }
    if (boundaries->hasBottomBoundary && !boundaries->hasLeftOrRightBoundary) JxrInverseTransformMathApplyAlternatePost2(firstStage - 16, firstStage + 48);
    JxrInverseTransformChroma422AlternatePlaneRestorePredictions(firstStage, secondStage, boundaries, after);
}

static Void JxrInverseTransformChroma422AlternatePlaneApplyFirstStageTransform(PixelI* firstStage, PixelI* secondStage,
    const JxrInverseTransformMacroblockGeometry* geometry, const JxrInverseTransformBoundaryContext* boundaries)
{
    Int offset;
    if (!geometry->isTop) for (offset = geometry->isLeft ? 112 : (boundaries->isLeftAdjacentToVerticalBoundary ? -80 : -16); offset < (boundaries->hasRightBoundary ? 48 : 112); offset += 64) strIDCT4x4Stage1(firstStage + offset);
    if (!geometry->isBottom) for (offset = geometry->isLeft ? 64 : (boundaries->isLeftAdjacentToVerticalBoundary ? -128 : -64); offset < (boundaries->hasRightBoundary ? 0 : 64); offset += 64) { strIDCT4x4Stage1(secondStage + offset); strIDCT4x4Stage1(secondStage + offset + 16); strIDCT4x4Stage1(secondStage + offset + 32); }
}

static Void JxrInverseTransformChroma422AlternatePlaneApplyPair(PixelI* edge)
{
    JxrInverseTransformMathApplyAlternatePost4(edge, edge - 2, edge + 6, edge + 8);
    JxrInverseTransformMathApplyAlternatePost4(edge + 1, edge - 1, edge + 7, edge + 9);
}

static Void JxrInverseTransformChroma422AlternatePlaneApplyHorizontalBoundary(PixelI* edge, Bool left,
    const JxrInverseTransformBoundaryContext* boundaries)
{
    Int offset;
    for (offset = left ? 0 : -128; offset < (boundaries->hasRightBoundary ? -64 : 0); offset += 64) {
        JxrInverseTransformMathApplyAlternatePost4(edge + offset, edge + offset - 1, edge + offset + 59, edge + offset + 60);
        JxrInverseTransformMathApplyAlternatePost4(edge + offset + 2, edge + offset + 1, edge + offset + 61, edge + offset + 62);
    }
}

static Void JxrInverseTransformChroma422AlternatePlaneApplyFirstStageOverlap(PixelI* firstStage, PixelI* secondStage,
    const JxrInverseTransformMacroblockGeometry* geometry, const JxrInverseTransformBoundaryContext* boundaries)
{
    Int offset;
    if (geometry->overlap == OL_NONE) return;
    if (boundaries->hasTopBoundary && boundaries->isLeftAdjacentToVerticalBoundary) JxrInverseTransformMathApplyAlternatePost4(secondStage - 128, secondStage - 127, secondStage - 126, secondStage - 125);
    if (boundaries->hasTopBoundary && boundaries->hasRightBoundary) JxrInverseTransformMathApplyAlternatePost4(secondStage - 59, secondStage - 60, secondStage - 57, secondStage - 58);
    if (boundaries->hasBottomBoundary && boundaries->isLeftAdjacentToVerticalBoundary) JxrInverseTransformMathApplyAlternatePost4(firstStage - 70, firstStage - 69, firstStage - 72, firstStage - 71);
    if (boundaries->hasBottomBoundary && boundaries->hasRightBoundary) JxrInverseTransformMathApplyAlternatePost4(firstStage - 1, firstStage - 2, firstStage - 3, firstStage - 4);
    if (!geometry->isTop) {
        if (boundaries->isLeftAdjacentToVerticalBoundary) JxrInverseTransformChroma422AlternatePlaneApplyPair(firstStage - 86);
        if (boundaries->hasRightBoundary) JxrInverseTransformChroma422AlternatePlaneApplyPair(firstStage - 18);
        for (offset = geometry->isLeft ? 0 : -128; offset < (boundaries->hasRightBoundary ? -64 : 0); offset += 64) strPost4x4Stage1_alternate(firstStage + offset + 32, 0);
    }
    if (!geometry->isBottom) {
        if (boundaries->isLeftAdjacentToVerticalBoundary) { JxrInverseTransformChroma422AlternatePlaneApplyPair(secondStage - 118); JxrInverseTransformChroma422AlternatePlaneApplyPair(secondStage - 102); }
        if (boundaries->hasRightBoundary) { JxrInverseTransformChroma422AlternatePlaneApplyPair(secondStage - 50); JxrInverseTransformChroma422AlternatePlaneApplyPair(secondStage - 34); }
        for (offset = geometry->isLeft ? 0 : -128; offset < (boundaries->hasRightBoundary ? -64 : 0); offset += 64) { strPost4x4Stage1_alternate(secondStage + offset, 0); strPost4x4Stage1_alternate(secondStage + offset + 16, 0); }
    }
    if (boundaries->hasTopOrBottomBoundary) {
        if (boundaries->hasTopBoundary) JxrInverseTransformChroma422AlternatePlaneApplyHorizontalBoundary(secondStage + 5, geometry->isLeft, boundaries);
        if (boundaries->hasBottomBoundary) JxrInverseTransformChroma422AlternatePlaneApplyHorizontalBoundary(firstStage + 61, geometry->isLeft, boundaries);
    } else {
        if (boundaries->isLeftAdjacentToVerticalBoundary) {
            JxrInverseTransformMathApplyAlternatePost4(firstStage - 70, firstStage - 72, secondStage - 128, secondStage - 126);
            JxrInverseTransformMathApplyAlternatePost4(firstStage - 69, firstStage - 71, secondStage - 127, secondStage - 125);
        }
        if (boundaries->hasRightBoundary) {
            JxrInverseTransformMathApplyAlternatePost4(firstStage - 2, firstStage - 4, secondStage - 60, secondStage - 58);
            JxrInverseTransformMathApplyAlternatePost4(firstStage - 1, firstStage - 3, secondStage - 59, secondStage - 57);
        }
        for (offset = geometry->isLeft ? 0 : -128; offset < (boundaries->hasRightBoundary ? -64 : 0); offset += 64) strPost4x4Stage1Split_alternate(firstStage + offset + 48, secondStage + offset, 0);
    }
}

Void JxrInverseTransformChroma422AlternatePlaneApply(const JxrInverseTransformPlaneContext* plane,
    const JxrInverseTransformMacroblockGeometry* geometry, const JxrInverseTransformBoundaryContext* boundaries,
    Bool usesScaledArithmetic, PixelI* predictionBefore, PixelI* predictionAfter)
{
    PixelI* firstStage = plane->buffers.firstStage;
    PixelI* secondStage = plane->buffers.secondStage;
    JxrInverseTransformChroma422AlternatePlaneApplySecondStageTransform(secondStage, geometry, usesScaledArithmetic);
    JxrInverseTransformChroma422AlternatePlaneApplySecondStageOverlap(firstStage, secondStage, geometry, boundaries, predictionBefore, predictionAfter);
    if (geometry->thumbnailScale >= 4) return;
    JxrInverseTransformChroma422AlternatePlaneApplyFirstStageTransform(firstStage, secondStage, geometry, boundaries);
    JxrInverseTransformChroma422AlternatePlaneApplyFirstStageOverlap(firstStage, secondStage, geometry, boundaries);
}
