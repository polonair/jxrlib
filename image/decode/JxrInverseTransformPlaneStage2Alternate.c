#include "JxrInverseTransformPlaneStage2Alternate.h"

#include "JxrInverseTransformMath.h"
#include "strTransform.h"

extern Void strPost4x4Stage2Split_alternate(PixelI* first, PixelI* second);

static Void JxrInverseTransformPlaneStage2AlternateApplyCornerOperators(
    PixelI* firstStage,
    PixelI* secondStage,
    const JxrInverseTransformBoundaryContext* boundaries)
{
    if (boundaries->hasTopBoundary && boundaries->hasLeftBoundary)
        JxrInverseTransformMathApplyAlternatePost4(secondStage, secondStage + 64,
            secondStage + 16, secondStage + 80);
    if (boundaries->hasTopBoundary && boundaries->hasRightBoundary)
        JxrInverseTransformMathApplyAlternatePost4(secondStage - 128, secondStage - 64,
            secondStage - 112, secondStage - 48);
    if (boundaries->hasBottomBoundary && boundaries->hasLeftBoundary)
        JxrInverseTransformMathApplyAlternatePost4(firstStage + 32, firstStage + 96,
            firstStage + 48, firstStage + 112);
    if (boundaries->hasBottomBoundary && boundaries->hasRightBoundary)
        JxrInverseTransformMathApplyAlternatePost4(firstStage - 96, firstStage - 32,
            firstStage - 80, firstStage - 16);
}

static Void JxrInverseTransformPlaneStage2AlternateApplySideOperators(
    PixelI* firstStage,
    PixelI* secondStage,
    const JxrInverseTransformBoundaryContext* boundaries)
{
    Int sideOffset;

    if (!boundaries->hasLeftOrRightBoundary || boundaries->hasTopOrBottomBoundary)
        return;

    if (boundaries->hasLeftBoundary) {
        sideOffset = 0;
        JxrInverseTransformMathApplyAlternatePost4(firstStage + sideOffset + 32,
            firstStage + sideOffset + 48, secondStage + sideOffset,
            secondStage + sideOffset + 16);
        JxrInverseTransformMathApplyAlternatePost4(firstStage + sideOffset + 96,
            firstStage + sideOffset + 112, secondStage + sideOffset + 64,
            secondStage + sideOffset + 80);
    }
    if (boundaries->hasRightBoundary) {
        sideOffset = -128;
        JxrInverseTransformMathApplyAlternatePost4(firstStage + sideOffset + 32,
            firstStage + sideOffset + 48, secondStage + sideOffset,
            secondStage + sideOffset + 16);
        JxrInverseTransformMathApplyAlternatePost4(firstStage + sideOffset + 96,
            firstStage + sideOffset + 112, secondStage + sideOffset + 64,
            secondStage + sideOffset + 80);
    }
}

static Void JxrInverseTransformPlaneStage2AlternateApplyTopBottomOperators(
    PixelI* firstStage,
    PixelI* secondStage,
    const JxrInverseTransformBoundaryContext* boundaries)
{
    PixelI* edge;

    if (boundaries->hasTopOrBottomBoundary && !boundaries->isVerticalTileBoundary) {
        if (boundaries->hasTopBoundary) {
            edge = secondStage;
            JxrInverseTransformMathApplyAlternatePost4(edge - 128, edge - 64, edge, edge + 64);
            JxrInverseTransformMathApplyAlternatePost4(edge - 112, edge - 48, edge + 16, edge + 80);
        }
        if (boundaries->hasBottomBoundary) {
            edge = firstStage + 32;
            JxrInverseTransformMathApplyAlternatePost4(edge - 128, edge - 64, edge, edge + 64);
            JxrInverseTransformMathApplyAlternatePost4(edge - 112, edge - 48, edge + 16, edge + 80);
        }
    }
    if (!boundaries->hasTopOrBottomBoundary && !boundaries->isVerticalTileBoundary)
        strPost4x4Stage2Split_alternate(firstStage, secondStage);
}

Void JxrInverseTransformPlaneStage2AlternateApply(
    PixelI* firstStage,
    PixelI* secondStage,
    OVERLAP overlap,
    Bool isLeftOrRight,
    const JxrInverseTransformBoundaryContext* boundaries)
{
    if (overlap != OL_TWO)
        return;

    JxrInverseTransformPlaneStage2AlternateApplyCornerOperators(
        firstStage, secondStage, boundaries);
    JxrInverseTransformPlaneStage2AlternateApplySideOperators(
        firstStage, secondStage, boundaries);

    if (!isLeftOrRight)
        JxrInverseTransformPlaneStage2AlternateApplyTopBottomOperators(
            firstStage, secondStage, boundaries);
}
