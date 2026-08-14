#include "JxrInverseTransformFullResolutionPlane.h"

#include "JxrInverseTransformPlaneStage1Normal.h"
#include "JxrInverseTransformPlaneStage2.h"
#include "JxrInverseTransformPlaneStage2Normal.h"

Void JxrInverseTransformFullResolutionPlaneApply(
    const JxrInverseTransformPlaneContext* plane,
    const JxrInverseTransformMacroblockGeometry* geometry,
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

    JxrInverseTransformPlaneStage2NormalApply(firstStage, secondStage, geometry);

    if (postProcessEnabled)
        postProcMB(postProcessInfo, firstStage, secondStage, macroblockColumn,
            plane->channelIndex, plane->directCurrentQuantizer);

    JxrInverseTransformPlaneStage1NormalApply(firstStage, secondStage, geometry->overlap,
        geometry->isLeft, geometry->isRight, geometry->isTop, geometry->isBottom,
        geometry->isTopOrBottom, geometry->isLeftOrRight, plane->highPassQuantizer,
        plane->isHighPassAbsent, geometry->thumbnailScale);

    if (postProcessEnabled && !geometry->isTopOrLeft)
        postProcBlock(postProcessInfo, firstStage, secondStage, macroblockColumn,
            plane->channelIndex, plane->lowPassQuantizer);
}
