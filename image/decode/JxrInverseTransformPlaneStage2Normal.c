#include "JxrInverseTransformPlaneStage2Normal.h"

#include "JxrInverseTransformMath.h"
#include "strTransform.h"

extern Void strPost4x4Stage2Split(PixelI* first, PixelI* second);

Void JxrInverseTransformPlaneStage2NormalApply(
    PixelI* firstStage,
    PixelI* secondStage,
    const JxrInverseTransformMacroblockGeometry* geometry)
{
    Int sideOffset;
    PixelI* edge;

    if (geometry->overlap != OL_TWO)
        return;

    if (geometry->isLeftOrRight && !geometry->isTopOrBottom) {
        sideOffset = geometry->isLeft ? 0 : -128;
        JxrInverseTransformMathApplyPost4(firstStage + sideOffset + 32,
            firstStage + sideOffset + 48, secondStage + sideOffset,
            secondStage + sideOffset + 16);
        JxrInverseTransformMathApplyPost4(firstStage + sideOffset + 96,
            firstStage + sideOffset + 112, secondStage + sideOffset + 64,
            secondStage + sideOffset + 80);
    }

    if (!geometry->isLeftOrRight) {
        if (geometry->isTopOrBottom) {
            edge = geometry->isTop ? secondStage : firstStage + 32;
            JxrInverseTransformMathApplyPost4(edge - 128, edge - 64, edge, edge + 64);
            JxrInverseTransformMathApplyPost4(edge - 112, edge - 48, edge + 16, edge + 80);
        }
        else {
            strPost4x4Stage2Split(firstStage, secondStage);
        }
    }
}
