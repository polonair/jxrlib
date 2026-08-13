#include "JxrForwardTransformMath.h"

Void JxrForwardTransformMathRotateHalf(PixelI* first, PixelI* second)
{
    *second -= (*first + 1) >> 1;
    *first += (*second + 1) >> 1;
}

Void JxrForwardTransformMathRotateThreeEighths(PixelI* first, PixelI* second)
{
    *second -= (*first * 3 + 4) >> 3;
    *first += (*second * 3 + 4) >> 3;
}
