#ifndef JXR_FORWARD_TRANSFORM_MATH_H
#define JXR_FORWARD_TRANSFORM_MATH_H

#include "windowsmediaphoto.h"

Void JxrForwardTransformMathRotateHalf(PixelI* first, PixelI* second);
Void JxrForwardTransformMathRotateThreeEighths(PixelI* first, PixelI* second);
Void JxrForwardTransformMathNormalizeBlock(
    PixelI* samples,
    Bool chroma,
    Int sampleCount,
    Int sampleStride);
Void JxrForwardTransformMathApplyDct2x2Down(
    PixelI* first,
    PixelI* second,
    PixelI* third,
    PixelI* fourth);
Void JxrForwardTransformMathApplyPre2(PixelI* first, PixelI* second);
Void JxrForwardTransformMathApplyPre2x2(
    PixelI* first,
    PixelI* second,
    PixelI* third,
    PixelI* fourth);
Void JxrForwardTransformMathApplyPre4(
    PixelI* first,
    PixelI* second,
    PixelI* third,
    PixelI* fourth);

#endif
