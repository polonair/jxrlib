#ifndef JXR_INVERSE_COLOR_TRANSFORM_H
#define JXR_INVERSE_COLOR_TRANSFORM_H

#include "windowsmediaphoto.h"

Void JxrInverseColorTransformApplyRgb(PixelI* red, PixelI* green, PixelI* blue);
Void JxrInverseColorTransformApplyCmyk(PixelI* cyan, PixelI* magenta,
    PixelI* yellow, PixelI* black);

#endif
