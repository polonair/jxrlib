#include "JxrInverseTransformPlaneStage1Normal.h"
#include "JxrInverseTransformMath.h"
#include "strTransform.h"

extern Void strIDCT4x4Stage1(PixelI* samples);
extern Void strPost4x4Stage1(PixelI* samples, Int offset, Int highPassQuantizer, Bool highPassAbsent);
extern Void strPost4x4Stage1Split(PixelI* first, PixelI* second, Int offset, Int highPassQuantizer, Bool highPassAbsent);

Void JxrInverseTransformPlaneStage1NormalApply(PixelI* p0, PixelI* p1,
    OVERLAP overlap, Bool left, Bool right, Bool top, Bool bottom,
    Bool topOrBottom, Bool leftOrRight, Int highPassQuantizer,
    Bool highPassAbsent, size_t thumbnailScale)
{
    PixelI* p;
    Int j;
    if (thumbnailScale >= 4) return;
    if (!top) for (j = left ? 32 : -96; j < (right ? 32 : 160); j += 64) { strIDCT4x4Stage1(p0 + j); strIDCT4x4Stage1(p0 + j + 16); }
    if (!bottom) for (j = left ? 0 : -128; j < (right ? 0 : 128); j += 64) { strIDCT4x4Stage1(p1 + j); strIDCT4x4Stage1(p1 + j + 16); }
    if (overlap == OL_NONE) return;
    if (leftOrRight) {
        j = left ? 10 : -50;
        if (!top) { p = p0 + 16 + j; JxrInverseTransformMathApplyPost4(p, p - 2, p + 6, p + 8); JxrInverseTransformMathApplyPost4(p + 1, p - 1, p + 7, p + 9); JxrInverseTransformMathApplyPost4(p + 16, p + 14, p + 22, p + 24); JxrInverseTransformMathApplyPost4(p + 17, p + 15, p + 23, p + 25); }
        if (!bottom) { p = p1 + j; JxrInverseTransformMathApplyPost4(p, p - 2, p + 6, p + 8); JxrInverseTransformMathApplyPost4(p + 1, p - 1, p + 7, p + 9); }
        if (!topOrBottom) { JxrInverseTransformMathApplyPost4(p0 + 48 + j, p0 + 46 + j, p1 - 10 + j, p1 - 8 + j); JxrInverseTransformMathApplyPost4(p0 + 49 + j, p0 + 47 + j, p1 - 9 + j, p1 - 7 + j); }
    }
    for (j = left ? 0 : -192; j < (right ? -64 : 64); j += 64) {
        if (top) { p = p1 + j; JxrInverseTransformMathApplyPost4(p + 5, p + 4, p + 64, p + 65); JxrInverseTransformMathApplyPost4(p + 7, p + 6, p + 66, p + 67); strPost4x4Stage1(p1 + j, 0, highPassQuantizer, highPassAbsent); }
        else if (bottom) { strPost4x4Stage1(p0 + 16 + j, 0, highPassQuantizer, highPassAbsent); strPost4x4Stage1(p0 + 32 + j, 0, highPassQuantizer, highPassAbsent); p = p0 + 48 + j; JxrInverseTransformMathApplyPost4(p + 15, p + 14, p + 74, p + 75); JxrInverseTransformMathApplyPost4(p + 13, p + 12, p + 72, p + 73); }
        else { strPost4x4Stage1(p0 + 16 + j, 0, highPassQuantizer, highPassAbsent); strPost4x4Stage1(p0 + 32 + j, 0, highPassQuantizer, highPassAbsent); strPost4x4Stage1Split(p0 + 48 + j, p1 + j, 0, highPassQuantizer, highPassAbsent); strPost4x4Stage1(p1 + j, 0, highPassQuantizer, highPassAbsent); }
    }
}
