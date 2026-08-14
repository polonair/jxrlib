#include "JxrInverseTransformChroma420Plane.h"

#include "JxrInverseTransformMath.h"
#include "JxrTransformMath.h"
#include "strTransform.h"

extern Void strIDCT4x4Stage1(PixelI* samples);
extern Void strPost4x4Stage1(PixelI* samples, Int offset, Int highPassQuantizer, Bool highPassAbsent);
extern Void strPost4x4Stage1Split(PixelI* first, PixelI* second, Int offset,
    Int highPassQuantizer, Bool highPassAbsent);

static Void JxrInverseTransformChroma420PlaneApplySecondStageTransform(
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

static Void JxrInverseTransformChroma420PlaneApplySecondStageOverlap(
    PixelI* firstStage,
    PixelI* secondStage,
    const JxrInverseTransformMacroblockGeometry* geometry)
{
    Int sideOffset;

    if (geometry->overlap != OL_TWO)
        return;

    if (geometry->isLeftOrRight && !geometry->isTopOrBottom) {
        sideOffset = geometry->isLeft ? 0 : -32;
        JxrInverseTransformMathApplyPost2(firstStage + sideOffset + 16,
            secondStage + sideOffset);
    }

    if (!geometry->isLeftOrRight) {
        if (geometry->isTopOrBottom) {
            PixelI* edge = geometry->isTop ? secondStage : firstStage + 16;
            JxrInverseTransformMathApplyPost2(edge - 32, edge);
        }
        else {
            JxrInverseTransformMathApplyPost2x2(firstStage - 16, firstStage + 16,
                secondStage - 32, secondStage);
        }
    }
}

static Void JxrInverseTransformChroma420PlaneApplyFirstStageTransform(
    PixelI* firstStage,
    PixelI* secondStage,
    const JxrInverseTransformMacroblockGeometry* geometry)
{
    Int offset;

    if (!geometry->isTop) {
        for (offset = geometry->isLeft ? 16 : -16;
            offset < (geometry->isRight ? 16 : 48); offset += 32)
            strIDCT4x4Stage1(firstStage + offset);
    }
    if (!geometry->isBottom) {
        for (offset = geometry->isLeft ? 0 : -32;
            offset < (geometry->isRight ? 0 : 32); offset += 32)
            strIDCT4x4Stage1(secondStage + offset);
    }
}

static Void JxrInverseTransformChroma420PlaneApplyFirstStageOverlap(
    PixelI* firstStage,
    PixelI* secondStage,
    const JxrInverseTransformPlaneContext* plane,
    const JxrInverseTransformMacroblockGeometry* geometry)
{
    Int offset;
    PixelI* edge;

    if (geometry->overlap == OL_NONE)
        return;

    if (!geometry->isLeft && !geometry->isTop) {
        if (geometry->isBottom) {
            for (offset = -48; offset < (geometry->isRight ? -16 : 16); offset += 32) {
                edge = firstStage + offset;
                JxrInverseTransformMathApplyPost4(edge + 15, edge + 14, edge + 42, edge + 43);
                JxrInverseTransformMathApplyPost4(edge + 13, edge + 12, edge + 40, edge + 41);
            }
        }
        else {
            for (offset = -48; offset < (geometry->isRight ? -16 : 16); offset += 32)
                strPost4x4Stage1Split(firstStage + offset, secondStage - 16 + offset, 32,
                    plane->highPassQuantizer, plane->isHighPassAbsent);
        }

        if (geometry->isRight) {
            if (!geometry->isBottom) {
                JxrInverseTransformMathApplyPost4(firstStage - 2, firstStage - 4,
                    secondStage - 28, secondStage - 26);
                JxrInverseTransformMathApplyPost4(firstStage - 1, firstStage - 3,
                    secondStage - 27, secondStage - 25);
            }
            JxrInverseTransformMathApplyPost4(firstStage - 18, firstStage - 20,
                firstStage - 12, firstStage - 10);
            JxrInverseTransformMathApplyPost4(firstStage - 17, firstStage - 19,
                firstStage - 11, firstStage - 9);
        }
        else {
            strPost4x4Stage1(firstStage - 32, 32,
                plane->highPassQuantizer, plane->isHighPassAbsent);
        }
        strPost4x4Stage1(firstStage - 64, 32,
            plane->highPassQuantizer, plane->isHighPassAbsent);
    }
    else if (geometry->isTop) {
        for (offset = geometry->isLeft ? 0 : -64;
            offset < (geometry->isRight ? -32 : 0); offset += 32) {
            edge = secondStage + offset + 4;
            JxrInverseTransformMathApplyPost4(edge + 1, edge, edge + 28, edge + 29);
            JxrInverseTransformMathApplyPost4(edge + 3, edge + 2, edge + 30, edge + 31);
        }
    }
    else if (geometry->isLeft) {
        if (!geometry->isBottom) {
            JxrInverseTransformMathApplyPost4(firstStage + 26, firstStage + 24,
                secondStage, secondStage + 2);
            JxrInverseTransformMathApplyPost4(firstStage + 27, firstStage + 25,
                secondStage + 1, secondStage + 3);
        }
        JxrInverseTransformMathApplyPost4(firstStage + 10, firstStage + 8,
            firstStage + 16, firstStage + 18);
        JxrInverseTransformMathApplyPost4(firstStage + 11, firstStage + 9,
            firstStage + 17, firstStage + 19);
    }
}

Void JxrInverseTransformChroma420PlaneApply(
    const JxrInverseTransformPlaneContext* plane,
    const JxrInverseTransformMacroblockGeometry* geometry,
    Bool usesScaledArithmetic)
{
    PixelI* firstStage = plane->buffers.firstStage;
    PixelI* secondStage = plane->buffers.secondStage;

    JxrInverseTransformChroma420PlaneApplySecondStageTransform(
        secondStage, geometry, usesScaledArithmetic);
    JxrInverseTransformChroma420PlaneApplySecondStageOverlap(
        firstStage, secondStage, geometry);
    if (geometry->thumbnailScale >= 4)
        return;
    JxrInverseTransformChroma420PlaneApplyFirstStageTransform(
        firstStage, secondStage, geometry);
    JxrInverseTransformChroma420PlaneApplyFirstStageOverlap(
        firstStage, secondStage, plane, geometry);
}
