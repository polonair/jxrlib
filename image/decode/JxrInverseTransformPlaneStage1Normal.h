#ifndef JXR_INVERSE_TRANSFORM_PLANE_STAGE1_NORMAL_H
#define JXR_INVERSE_TRANSFORM_PLANE_STAGE1_NORMAL_H

#include "strcodec.h"

Void JxrInverseTransformPlaneStage1NormalApply(PixelI* firstStage, PixelI* secondStage,
    OVERLAP overlap, Bool left, Bool right, Bool top, Bool bottom,
    Bool topOrBottom, Bool leftOrRight, Int highPassQuantizer,
    Bool highPassAbsent, size_t thumbnailScale);

#endif
