#ifndef JXR_INVERSE_TRANSFORM_FULL_RESOLUTION_PLANE_H
#define JXR_INVERSE_TRANSFORM_FULL_RESOLUTION_PLANE_H

#include "JxrInverseTransformPlaneContext.h"
#include "JxrInverseTransformMacroblockGeometry.h"

/* Coordinates transform and optional post-processing for one normal full-resolution plane. */
Void JxrInverseTransformFullResolutionPlaneApply(
    const JxrInverseTransformPlaneContext* plane,
    const JxrInverseTransformMacroblockGeometry* geometry,
    Bool usesScaledArithmetic,
    Bool postProcessEnabled,
    struct tagPostProcInfo* postProcessInfo[MAX_CHANNELS][2],
    size_t macroblockColumn);

#endif
