#ifndef JXR_INVERSE_TRANSFORM_PLANE_STAGE2_H
#define JXR_INVERSE_TRANSFORM_PLANE_STAGE2_H

#include "strcodec.h"

Void JxrInverseTransformPlaneStage2Apply(
    PixelI* secondStage,
    Bool chroma,
    Bool usesScaledArithmetic);

#endif
