#ifndef JXR_INVERSE_TRANSFORM_PLANE_STAGE1_ALTERNATE_H
#define JXR_INVERSE_TRANSFORM_PLANE_STAGE1_ALTERNATE_H

#include "JxrInverseTransformBoundaryContext.h"

Void JxrInverseTransformPlaneStage1AlternateApply(PixelI* firstStage, PixelI* secondStage,
    OVERLAP overlap, Bool left, Bool right, Bool top, Bool bottom,
    const JxrInverseTransformBoundaryContext* boundaries, size_t thumbnailScale);

#endif
