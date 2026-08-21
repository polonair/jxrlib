#ifndef JXR_ENCODER_COLOR_TRANSFORM_H
#define JXR_ENCODER_COLOR_TRANSFORM_H

#include "windowsmediaphoto.h"

Void JxrEncoderColorTransformApplyRgb(PixelI* red, PixelI* green, PixelI* blue);
Void JxrEncoderColorTransformApplyCmyk(PixelI* cyan, PixelI* magenta,
    PixelI* yellow, PixelI* black);

#endif
