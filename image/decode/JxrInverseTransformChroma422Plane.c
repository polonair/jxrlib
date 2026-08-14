#include "JxrInverseTransformChroma422Plane.h"

#include "JxrInverseTransformMath.h"
#include "JxrTransformMath.h"
#include "strTransform.h"

extern Void strIDCT4x4Stage1(PixelI* samples);
extern Void strPost4x4Stage1(PixelI* samples, Int offset, Int highPassQuantizer, Bool highPassAbsent);
extern Void strPost4x4Stage1Split(PixelI* first, PixelI* second, Int offset,
    Int highPassQuantizer, Bool highPassAbsent);

static Void JxrInverseTransformChroma422PlaneApplySecondStageTransform(
    PixelI* secondStage,
    const JxrInverseTransformMacroblockGeometry* geometry,
    Bool usesScaledArithmetic)
{
    if (geometry->isBottomOrRight || geometry->thumbnailScale >= 16)
        return;

    /* 1D lossless horizontal transform. */
    secondStage[0] -= (secondStage[32] + 1) >> 1;
    secondStage[32] += secondStage[0];

    if (usesScaledArithmetic) {
        JxrInverseTransformMathApplyScaledDct2x2Down(secondStage, secondStage + 64,
            secondStage + 16, secondStage + 80);
        JxrInverseTransformMathApplyScaledDct2x2Down(secondStage + 32, secondStage + 96,
            secondStage + 48, secondStage + 112);
    }
    else {
        JxrTransformMathApplyDct2x2Down(secondStage, secondStage + 64,
            secondStage + 16, secondStage + 80);
        JxrTransformMathApplyDct2x2Down(secondStage + 32, secondStage + 96,
            secondStage + 48, secondStage + 112);
    }
}

static Void JxrInverseTransformChroma422PlaneApplySecondStageOverlap(
    PixelI* firstStage,
    PixelI* secondStage,
    const JxrInverseTransformMacroblockGeometry* geometry)
{
    Int sideOffset;

    if (geometry->overlap != OL_TWO)
        return;

    if (!geometry->isBottom) {
        if (geometry->isLeftOrRight) {
            if (!geometry->isTop) {
                sideOffset = geometry->isLeft ? 0 : -64;
                JxrInverseTransformMathApplyPost2(firstStage + 48 + sideOffset,
                    secondStage + sideOffset);
            }
            sideOffset = geometry->isLeft ? 16 : -48;
            JxrInverseTransformMathApplyPost2(secondStage + sideOffset,
                secondStage + sideOffset + 16);
        }
        else {
            if (geometry->isTop)
                JxrInverseTransformMathApplyPost2(secondStage - 64, secondStage);
            else
                JxrInverseTransformMathApplyPost2x2(firstStage - 16, firstStage + 48,
                    secondStage - 64, secondStage);
            JxrInverseTransformMathApplyPost2x2(secondStage - 48, secondStage + 16,
                secondStage - 32, secondStage + 32);
        }
    }
    else if (!geometry->isLeftOrRight) {
        JxrInverseTransformMathApplyPost2(firstStage - 16, firstStage + 48);
    }
}

static Void JxrInverseTransformChroma422PlaneApplyFirstStageTransform(
    PixelI* firstStage,
    PixelI* secondStage,
    const JxrInverseTransformMacroblockGeometry* geometry)
{
    Int offset;

    if (!geometry->isTop) {
        for (offset = geometry->isLeft ? 48 : -16;
            offset < (geometry->isRight ? 48 : 112); offset += 64)
            strIDCT4x4Stage1(firstStage + offset);
    }
    if (!geometry->isBottom) {
        for (offset = geometry->isLeft ? 0 : -64;
            offset < (geometry->isRight ? 0 : 64); offset += 64) {
            strIDCT4x4Stage1(secondStage + offset);
            strIDCT4x4Stage1(secondStage + offset + 16);
            strIDCT4x4Stage1(secondStage + offset + 32);
        }
    }
}

static Void JxrInverseTransformChroma422PlaneApplyFirstStageOverlap(
    PixelI* firstStage,
    PixelI* secondStage,
    const JxrInverseTransformPlaneContext* plane,
    const JxrInverseTransformMacroblockGeometry* geometry)
{
    Int offset;
    PixelI* edge;

    if (geometry->overlap == OL_NONE)
        return;

    if (!geometry->isTop) {
        if (geometry->isLeftOrRight) {
            offset = geometry->isLeft ? 42 : -18;
            edge = firstStage + offset;
            JxrInverseTransformMathApplyPost4(edge, edge - 2, edge + 6, edge + 8);
            JxrInverseTransformMathApplyPost4(edge + 1, edge - 1, edge + 7, edge + 9);
        }
        for (offset = geometry->isLeft ? 0 : -128;
            offset < (geometry->isRight ? -64 : 0); offset += 64)
            strPost4x4Stage1(firstStage + offset + 32, 0,
                plane->highPassQuantizer, plane->isHighPassAbsent);
    }

    if (!geometry->isBottom) {
        if (geometry->isLeftOrRight) {
            offset = geometry->isLeft ? 10 : -50;
            edge = secondStage + offset;
            JxrInverseTransformMathApplyPost4(edge, edge - 2, edge + 6, edge + 8);
            JxrInverseTransformMathApplyPost4(edge + 1, edge - 1, edge + 7, edge + 9);
            edge += 16;
            JxrInverseTransformMathApplyPost4(edge, edge - 2, edge + 6, edge + 8);
            JxrInverseTransformMathApplyPost4(edge + 1, edge - 1, edge + 7, edge + 9);
        }
        for (offset = geometry->isLeft ? 0 : -128;
            offset < (geometry->isRight ? -64 : 0); offset += 64) {
            strPost4x4Stage1(secondStage + offset, 0,
                plane->highPassQuantizer, plane->isHighPassAbsent);
            strPost4x4Stage1(secondStage + offset + 16, 0,
                plane->highPassQuantizer, plane->isHighPassAbsent);
        }
    }

    if (geometry->isTopOrBottom) {
        edge = geometry->isTop ? secondStage + 5 : firstStage + 61;
        for (offset = geometry->isLeft ? 0 : -128;
            offset < (geometry->isRight ? -64 : 0); offset += 64) {
            JxrInverseTransformMathApplyPost4(edge + offset, edge + offset - 1,
                edge + offset + 59, edge + offset + 60);
            JxrInverseTransformMathApplyPost4(edge + offset + 2, edge + offset + 1,
                edge + offset + 61, edge + offset + 62);
        }
    }
    else {
        if (geometry->isLeftOrRight) {
            offset = geometry->isLeft ? 0 : -60;
            JxrInverseTransformMathApplyPost4(firstStage + offset + 58,
                firstStage + offset + 56, secondStage + offset, secondStage + offset + 2);
            JxrInverseTransformMathApplyPost4(firstStage + offset + 59,
                firstStage + offset + 57, secondStage + offset + 1, secondStage + offset + 3);
        }
        for (offset = geometry->isLeft ? 0 : -128;
            offset < (geometry->isRight ? -64 : 0); offset += 64)
            strPost4x4Stage1Split(firstStage + offset + 48, secondStage + offset, 0,
                plane->highPassQuantizer, plane->isHighPassAbsent);
    }
}

Void JxrInverseTransformChroma422PlaneApply(
    const JxrInverseTransformPlaneContext* plane,
    const JxrInverseTransformMacroblockGeometry* geometry,
    Bool usesScaledArithmetic)
{
    PixelI* firstStage = plane->buffers.firstStage;
    PixelI* secondStage = plane->buffers.secondStage;

    JxrInverseTransformChroma422PlaneApplySecondStageTransform(
        secondStage, geometry, usesScaledArithmetic);
    JxrInverseTransformChroma422PlaneApplySecondStageOverlap(
        firstStage, secondStage, geometry);
    if (geometry->thumbnailScale >= 4)
        return;
    JxrInverseTransformChroma422PlaneApplyFirstStageTransform(
        firstStage, secondStage, geometry);
    JxrInverseTransformChroma422PlaneApplyFirstStageOverlap(
        firstStage, secondStage, plane, geometry);
}
