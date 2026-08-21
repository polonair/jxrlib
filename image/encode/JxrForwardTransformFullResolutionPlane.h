#ifndef JXR_FORWARD_TRANSFORM_FULL_RESOLUTION_PLANE_H
#define JXR_FORWARD_TRANSFORM_FULL_RESOLUTION_PLANE_H

#include "JxrForwardTransformBoundaryContext.h"

/* Applies both overlap/DCT levels to one full-resolution forward plane. */
Void JxrForwardTransformFullResolutionPlaneApply(
    PixelI* firstStage,
    PixelI* secondStage,
    Bool isChroma,
    const JxrForwardTransformMacroblockGeometry* geometry,
    const JxrForwardTransformBoundaryContext* boundaries,
    Bool usesScaledArithmetic);

#endif
