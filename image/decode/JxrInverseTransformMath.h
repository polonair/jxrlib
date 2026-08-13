#ifndef JXR_INVERSE_TRANSFORM_MATH_H
#define JXR_INVERSE_TRANSFORM_MATH_H

#include "windowsmediaphoto.h"

Void JxrInverseTransformMathRotateHalf(PixelI* first, PixelI* second);
Void JxrInverseTransformMathRotateThreeEighths(PixelI* first, PixelI* second);
Bool JxrInverseTransformMathShouldCompensateDc(
    Int directCurrent,
    Int highPassQuantizer,
    Bool highPassAbsent);
Void JxrInverseTransformMathApplyDcCompensation(
    PixelI* topLeft,
    PixelI* topRight,
    PixelI* bottomLeft,
    PixelI* bottomRight,
    Int directCurrent);
Int JxrInverseTransformMathClipDcWithAlternate(
    Int directCurrent,
    Int alternateCurrent);
Int JxrInverseTransformMathApplyConditionalDcCompensation(
    PixelI* topLeft,
    PixelI* topRight,
    PixelI* bottomLeft,
    PixelI* bottomRight,
    Int directCurrent,
    Int highPassQuantizer,
    Bool highPassAbsent);
Void JxrInverseTransformMathApplyPost4(
    PixelI* first,
    PixelI* second,
    PixelI* third,
    PixelI* fourth);

#endif
