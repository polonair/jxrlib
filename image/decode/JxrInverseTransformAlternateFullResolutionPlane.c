#include "JxrInverseTransformAlternateFullResolutionPlane.h"

#include "JxrInverseTransformPlaneStage1Alternate.h"
#include "JxrInverseTransformPlaneStage2.h"
#include "JxrInverseTransformPlaneStage2Alternate.h"

Void JxrInverseTransformAlternateFullResolutionPlaneApply(
    const JxrInverseTransformPlaneContext* plane,
    const JxrInverseTransformMacroblockGeometry* geometry,
    const JxrInverseTransformBoundaryContext* boundaries,
    Bool usesScaledArithmetic,
    Bool postProcessEnabled,
    struct tagPostProcInfo* postProcessInfo[MAX_CHANNELS][2],
    size_t macroblockColumn)
{
    PixelI* firstStage = plane->buffers.firstStage;
    PixelI* secondStage = plane->buffers.secondStage;

    if (!geometry->isBottomOrRight) {
        if (postProcessEnabled)
            updatePostProcInfo(postProcessInfo, secondStage, macroblockColumn, plane->channelIndex);
        JxrInverseTransformPlaneStage2Apply(secondStage, plane->channelIndex != 0,
            usesScaledArithmetic);
    }

    JxrInverseTransformPlaneStage2AlternateApply(firstStage, secondStage, geometry->overlap,
        geometry->isLeftOrRight, boundaries);

    if (postProcessEnabled)
        postProcMB(postProcessInfo, firstStage, secondStage, macroblockColumn,
            plane->channelIndex, plane->directCurrentQuantizer);

    JxrInverseTransformPlaneStage1AlternateApply(firstStage, secondStage, geometry->overlap,
        geometry->isLeft, geometry->isRight, geometry->isTop, geometry->isBottom,
        boundaries, geometry->thumbnailScale);

    if (postProcessEnabled && !geometry->isTopOrLeft)
        postProcBlock(postProcessInfo, firstStage, secondStage, macroblockColumn,
            plane->channelIndex, plane->lowPassQuantizer);
}
