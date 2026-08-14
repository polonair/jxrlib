#include "JxrTransformMath.h"

static Void JxrTransformMathApplyDct2x2(
    PixelI* first,
    PixelI* second,
    PixelI* third,
    PixelI* fourth,
    Bool roundUp)
{
    PixelI firstValue = *first;
    PixelI secondValue = *second;
    PixelI thirdInput = *third;
    PixelI fourthValue = *fourth;
    PixelI average;
    PixelI thirdValue;

    firstValue += fourthValue;
    secondValue -= thirdInput;
    average = firstValue - secondValue;
    if (roundUp) {
        average += 1;
    }
    average >>= 1;
    thirdValue = average - fourthValue;
    fourthValue = average - thirdInput;
    firstValue -= fourthValue;
    secondValue += thirdValue;

    *first = firstValue;
    *second = secondValue;
    *third = thirdValue;
    *fourth = fourthValue;
}

Void JxrTransformMathApplyDct2x2Down(
    PixelI* first,
    PixelI* second,
    PixelI* third,
    PixelI* fourth)
{
    JxrTransformMathApplyDct2x2(first, second, third, fourth, FALSE);
}

Void JxrTransformMathApplyDct2x2Up(
    PixelI* first,
    PixelI* second,
    PixelI* third,
    PixelI* fourth)
{
    JxrTransformMathApplyDct2x2(first, second, third, fourth, TRUE);
}

Void JxrTransformMathApplyFourButterfly(
    PixelI* buffer,
    const Int* offsets)
{
    Int group;

    for (group = 0; group < 4; ++group) {
        Int offsetIndex = group * 4;

        JxrTransformMathApplyDct2x2Down(
            &buffer[offsets[offsetIndex]],
            &buffer[offsets[offsetIndex + 1]],
            &buffer[offsets[offsetIndex + 2]],
            &buffer[offsets[offsetIndex + 3]]);
    }
}
