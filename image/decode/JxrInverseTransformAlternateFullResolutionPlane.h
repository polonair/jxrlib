#ifndef JXR_INVERSE_TRANSFORM_ALTERNATE_FULL_RESOLUTION_PLANE_H
#define JXR_INVERSE_TRANSFORM_ALTERNATE_FULL_RESOLUTION_PLANE_H

#include "JxrInverseTransformBoundaryContext.h"
#include "JxrInverseTransformPlaneContext.h"

/* Coordinates transform and optional post-processing for one hard-tile full-resolution plane. */
Void JxrInverseTransformAlternateFullResolutionPlaneApply(
    const JxrInverseTransformPlaneContext* plane,
    const JxrInverseTransformMacroblockGeometry* geometry,
    const JxrInverseTransformBoundaryContext* boundaries,
    Bool usesScaledArithmetic,
    Bool postProcessEnabled,
    struct tagPostProcInfo* postProcessInfo[MAX_CHANNELS][2],
    size_t macroblockColumn);

#endif
