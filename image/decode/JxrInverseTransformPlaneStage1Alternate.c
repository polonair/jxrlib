#include "JxrInverseTransformPlaneStage1Alternate.h"
#include "JxrInverseTransformMath.h"
#include "strTransform.h"

extern Void strIDCT4x4Stage1(PixelI* samples);
extern Void strPost4x4Stage1_alternate(PixelI* samples, Int offset);
extern Void strPost4x4Stage1Split_alternate(PixelI* first, PixelI* second, Int offset);

static Void JxrInverseTransformPlaneStage1AlternateApplyEdge(PixelI* p0, PixelI* p1,
    Bool top, Bool bottom, const JxrInverseTransformBoundaryContext* boundaries, Int offset)
{
    PixelI* p;
    if (!top) { p = p0 + 16 + offset; JxrInverseTransformMathApplyAlternatePost4(p, p - 2, p + 6, p + 8); JxrInverseTransformMathApplyAlternatePost4(p + 1, p - 1, p + 7, p + 9); JxrInverseTransformMathApplyAlternatePost4(p + 16, p + 14, p + 22, p + 24); JxrInverseTransformMathApplyAlternatePost4(p + 17, p + 15, p + 23, p + 25); }
    if (!bottom) { p = p1 + offset; JxrInverseTransformMathApplyAlternatePost4(p, p - 2, p + 6, p + 8); JxrInverseTransformMathApplyAlternatePost4(p + 1, p - 1, p + 7, p + 9); }
    if (!boundaries->hasTopOrBottomBoundary) { JxrInverseTransformMathApplyAlternatePost4(p0 + 48 + offset, p0 + 46 + offset, p1 - 10 + offset, p1 - 8 + offset); JxrInverseTransformMathApplyAlternatePost4(p0 + 49 + offset, p0 + 47 + offset, p1 - 9 + offset, p1 - 7 + offset); }
}

Void JxrInverseTransformPlaneStage1AlternateApply(PixelI* p0, PixelI* p1,
    OVERLAP overlap, Bool left, Bool right, Bool top, Bool bottom,
    const JxrInverseTransformBoundaryContext* boundaries, size_t thumbnailScale)
{
    PixelI* p;
    Int j;
    if (thumbnailScale >= 4) return;
    if (!top) for (j = left ? 32 : -96; j < (right ? 32 : 160); j += 64) { strIDCT4x4Stage1(p0 + j); strIDCT4x4Stage1(p0 + j + 16); }
    if (!bottom) for (j = left ? 0 : -128; j < (right ? 0 : 128); j += 64) { strIDCT4x4Stage1(p1 + j); strIDCT4x4Stage1(p1 + j + 16); }
    if (overlap == OL_NONE) return;
    if (boundaries->hasLeftOrRightBoundary) {
        if (boundaries->hasTopBoundary && boundaries->hasLeftBoundary) JxrInverseTransformMathApplyAlternatePost4(p1, p1 + 1, p1 + 2, p1 + 3);
        if (boundaries->hasTopBoundary && boundaries->hasRightBoundary) JxrInverseTransformMathApplyAlternatePost4(p1 - 59, p1 - 60, p1 - 57, p1 - 58);
        if (boundaries->hasBottomBoundary && boundaries->hasLeftBoundary) JxrInverseTransformMathApplyAlternatePost4(p0 + 58, p0 + 59, p0 + 56, p0 + 57);
        if (boundaries->hasBottomBoundary && boundaries->hasRightBoundary) JxrInverseTransformMathApplyAlternatePost4(p0 - 1, p0 - 2, p0 - 3, p0 - 4);
        if (boundaries->hasLeftBoundary) JxrInverseTransformPlaneStage1AlternateApplyEdge(p0, p1, top, bottom, boundaries, 10);
        if (boundaries->hasRightBoundary) JxrInverseTransformPlaneStage1AlternateApplyEdge(p0, p1, top, bottom, boundaries, -50);
    }
    for (j = left ? 0 : -192; j < (right ? -64 : 64); j += 64) {
        if (boundaries->hasTopBoundary && (!boundaries->isVerticalTileBoundary || j != -64)) { p = p1 + j; JxrInverseTransformMathApplyAlternatePost4(p + 5, p + 4, p + 64, p + 65); JxrInverseTransformMathApplyAlternatePost4(p + 7, p + 6, p + 66, p + 67); strPost4x4Stage1_alternate(p1 + j, 0); }
        if (boundaries->hasBottomBoundary && (!boundaries->isVerticalTileBoundary || j != -64)) { strPost4x4Stage1_alternate(p0 + 16 + j, 0); strPost4x4Stage1_alternate(p0 + 32 + j, 0); p = p0 + 48 + j; JxrInverseTransformMathApplyAlternatePost4(p + 15, p + 14, p + 74, p + 75); JxrInverseTransformMathApplyAlternatePost4(p + 13, p + 12, p + 72, p + 73); }
        if (!boundaries->hasTopOrBottomBoundary && (!boundaries->isVerticalTileBoundary || j != -64)) { strPost4x4Stage1_alternate(p0 + 16 + j, 0); strPost4x4Stage1_alternate(p0 + 32 + j, 0); strPost4x4Stage1Split_alternate(p0 + 48 + j, p1 + j, 0); strPost4x4Stage1_alternate(p1 + j, 0); }
    }
}
