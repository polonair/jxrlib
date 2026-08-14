#ifndef JXR_INVERSE_TRANSFORM_PLANE_STAGE2_NORMAL_H
#define JXR_INVERSE_TRANSFORM_PLANE_STAGE2_NORMAL_H

#include "JxrInverseTransformMacroblockGeometry.h"

/* Applies normal full-resolution stage-2 overlap after inverse DCT. */
Void JxrInverseTransformPlaneStage2NormalApply(
    PixelI* firstStage,
    PixelI* secondStage,
    const JxrInverseTransformMacroblockGeometry* geometry);

#endif
