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

Void JxrForwardTransformMathNormalizeBlock(
    PixelI* samples,
    Bool chroma,
    Int sampleCount,
    Int sampleStride)
{
    Int sampleIndex;

    if (!chroma) {
        return;
    }

    for (sampleIndex = 0; sampleIndex < sampleCount; sampleIndex += sampleStride) {
        samples[sampleIndex] >>= 1;
    }
}

Void JxrForwardTransformMathApplyDct2x2Down(
    PixelI* first,
    PixelI* second,
    PixelI* third,
    PixelI* fourth)
{
    PixelI firstValue = *first >> 1;
    PixelI secondValue = *second >> 1;
    PixelI thirdInput = *third >> 1;
    PixelI fourthValue = *fourth >> 1;
    PixelI thirdValue;
    PixelI middle;

    firstValue += fourthValue;
    secondValue -= thirdInput;
    middle = (firstValue - secondValue) >> 1;
    thirdValue = middle - fourthValue;
    fourthValue = middle - thirdInput;
    firstValue -= fourthValue;
    secondValue += thirdValue;

    *first = firstValue;
    *second = secondValue;
    *third = thirdValue;
    *fourth = fourthValue;
}
