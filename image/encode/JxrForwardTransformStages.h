#ifndef JXR_FORWARD_TRANSFORM_STAGES_H
#define JXR_FORWARD_TRANSFORM_STAGES_H

#include "windowsmediaphoto.h"

Void JxrForwardTransformStagesApplyStage1Dct(PixelI* samples);
Void JxrForwardTransformStagesApplyStage2Dct(PixelI* samples);
Void JxrForwardTransformStagesApplyPreStage1Split(
    PixelI* firstStage,
    PixelI* secondStage,
    Int offset);
Void JxrForwardTransformStagesApplyPreStage2Split(
    PixelI* firstStage,
    PixelI* secondStage);

#endif
