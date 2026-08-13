#include "JxrInverseTransformMath.h"

Void JxrInverseTransformMathRotateHalf(PixelI* first, PixelI* second)
{
    *first -= (*second + 1) >> 1;
    *second += (*first + 1) >> 1;
}

Void JxrInverseTransformMathRotateThreeEighths(PixelI* first, PixelI* second)
{
    *first -= (*second * 3 + 4) >> 3;
    *second += (*first * 3 + 4) >> 3;
}

Bool JxrInverseTransformMathShouldCompensateDc(
    Int directCurrent,
    Int highPassQuantizer,
    Bool highPassAbsent)
{
    Int absoluteDirectCurrent = directCurrent;

    if (highPassAbsent) {
        return TRUE;
    }
    if (highPassQuantizer <= 20) {
        return FALSE;
    }
    if (absoluteDirectCurrent < 0) {
        absoluteDirectCurrent = -absoluteDirectCurrent;
    }

    return absoluteDirectCurrent < highPassQuantizer;
}
