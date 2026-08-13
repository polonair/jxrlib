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
