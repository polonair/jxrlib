#ifndef JXR_TRANSFORM_MATH_H
#define JXR_TRANSFORM_MATH_H

#include "windowsmediaphoto.h"

Void JxrTransformMathApplyDct2x2Down(
    PixelI* first,
    PixelI* second,
    PixelI* third,
    PixelI* fourth);
Void JxrTransformMathApplyDct2x2Up(
    PixelI* first,
    PixelI* second,
    PixelI* third,
    PixelI* fourth);
Void JxrTransformMathApplyFourButterfly(
    PixelI* buffer,
    const Int* offsets);

#endif
