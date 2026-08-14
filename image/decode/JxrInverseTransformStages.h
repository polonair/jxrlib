#ifndef JXR_INVERSE_TRANSFORM_STAGES_H
#define JXR_INVERSE_TRANSFORM_STAGES_H

#include "strcodec.h"

Void JxrInverseTransformStagesApplyStage1Idct(PixelI* samples);
Void JxrInverseTransformStagesApplyStage1SplitNormal(PixelI* first, PixelI* second,
    Int offset, Int highPassQuantizer, Bool highPassAbsent);
Void JxrInverseTransformStagesApplyStage1SplitAlternate(PixelI* first, PixelI* second,
    Int offset);

#endif
