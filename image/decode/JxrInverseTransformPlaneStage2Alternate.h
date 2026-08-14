#ifndef JXR_INVERSE_TRANSFORM_PLANE_STAGE2_ALTERNATE_H
#define JXR_INVERSE_TRANSFORM_PLANE_STAGE2_ALTERNATE_H

#include "JxrInverseTransformBoundaryContext.h"

Void JxrInverseTransformPlaneStage2AlternateApply(
    PixelI* firstStage,
    PixelI* secondStage,
    OVERLAP overlap,
    Bool isLeftOrRight,
    const JxrInverseTransformBoundaryContext* boundaries);

#endif
