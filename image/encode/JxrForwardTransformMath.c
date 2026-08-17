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

Void JxrForwardTransformMathApplyPre2(PixelI* first, PixelI* second)
{
    PixelI firstValue = *first;
    PixelI secondValue = *second;

    secondValue -= (firstValue + 2) >> 2;
    firstValue -= (secondValue + 1) >> 1;
    firstValue -= secondValue >> 5;
    firstValue -= secondValue >> 9;
    firstValue -= secondValue >> 13;
    secondValue -= (firstValue + 2) >> 2;

    *first = firstValue;
    *second = secondValue;
}

Void JxrForwardTransformMathApplyPre2x2(
    PixelI* first,
    PixelI* second,
    PixelI* third,
    PixelI* fourth)
{
    PixelI firstValue = *first;
    PixelI secondValue = *second;
    PixelI thirdValue = *third;
    PixelI fourthValue = *fourth;

    firstValue += fourthValue;
    secondValue += thirdValue;
    fourthValue -= (firstValue + 1) >> 1;
    thirdValue -= (secondValue + 1) >> 1;

    secondValue -= (firstValue + 2) >> 2;
    firstValue -= (secondValue + 1) >> 1;
    firstValue -= secondValue >> 5;
    firstValue -= secondValue >> 9;
    firstValue -= secondValue >> 13;
    secondValue -= (firstValue + 2) >> 2;

    fourthValue += (firstValue + 1) >> 1;
    thirdValue += (secondValue + 1) >> 1;
    firstValue -= fourthValue;
    secondValue -= thirdValue;

    *first = firstValue;
    *second = secondValue;
    *third = thirdValue;
    *fourth = fourthValue;
}

static Void JxrForwardTransformMathApplyPre4Edge(PixelI* first, PixelI* fourth)
{
    PixelI firstValue = *first;
    PixelI fourthValue = -*fourth;

    firstValue -= fourthValue;
    fourthValue += firstValue >> 1;
    firstValue -= (fourthValue * 3 + 4) >> 3;
    fourthValue -= firstValue >> 7;
    fourthValue += firstValue >> 10;
    fourthValue -= (firstValue * 3) >> 4;
    firstValue -= (fourthValue * 3) >> 3;
    fourthValue = (firstValue >> 1) - fourthValue;
    firstValue -= fourthValue;

    *first = firstValue;
    *fourth = fourthValue;
}

Void JxrForwardTransformMathApplyPre4(
    PixelI* first,
    PixelI* second,
    PixelI* third,
    PixelI* fourth)
{
    PixelI firstValue = *first;
    PixelI secondValue = *second;
    PixelI thirdValue = *third;
    PixelI fourthValue = *fourth;

    firstValue += fourthValue;
    secondValue += thirdValue;
    fourthValue -= (firstValue + 1) >> 1;
    thirdValue -= (secondValue + 1) >> 1;
    JxrForwardTransformMathRotateHalf(&thirdValue, &fourthValue);
    JxrForwardTransformMathApplyPre4Edge(&firstValue, &fourthValue);
    JxrForwardTransformMathApplyPre4Edge(&secondValue, &thirdValue);
    fourthValue += (firstValue + 1) >> 1;
    thirdValue += (secondValue + 1) >> 1;
    firstValue -= fourthValue;
    secondValue -= thirdValue;

    *first = firstValue;
    *second = secondValue;
    *third = thirdValue;
    *fourth = fourthValue;
}

Void JxrForwardTransformMathApplyHst4(
    PixelI* first,
    PixelI* second,
    PixelI* third,
    PixelI* fourth)
{
    PixelI firstValue = *first;
    PixelI secondValue = *second;
    PixelI thirdValue = *fourth;
    PixelI fourthValue = *third;

    firstValue += thirdValue;
    secondValue -= fourthValue;
    thirdValue = ((firstValue - secondValue) >> 1) - thirdValue;
    fourthValue += secondValue >> 1;
    secondValue += thirdValue;
    firstValue -= (fourthValue * 3 + 4) >> 3;

    *first = firstValue;
    *second = secondValue;
    *third = thirdValue;
    *fourth = fourthValue;
}

Void JxrForwardTransformMathApplyHst1(PixelI* first, PixelI* fourth)
{
    PixelI firstValue = *first;
    PixelI fourthValue = *fourth;

    fourthValue -= firstValue >> 7;
    fourthValue += firstValue >> 10;
    fourthValue -= (firstValue * 3) >> 4;
    firstValue -= (fourthValue * 3) >> 3;
    fourthValue = (firstValue >> 1) - fourthValue;
    firstValue -= fourthValue;

    *first = firstValue;
    *fourth = fourthValue;
}

Void JxrForwardTransformMathApplyOddOdd(
    PixelI* first,
    PixelI* second,
    PixelI* third,
    PixelI* fourth)
{
    PixelI firstValue = *first;
    PixelI secondValue = -*second;
    PixelI thirdValue = -*third;
    PixelI fourthValue = *fourth;
    PixelI firstTemporary;
    PixelI secondTemporary;

    fourthValue += firstValue;
    thirdValue -= secondValue;
    firstValue -= (firstTemporary = fourthValue >> 1);
    secondValue += (secondTemporary = thirdValue >> 1);
    firstValue += (secondValue * 3 + 4) >> 3;
    secondValue -= (firstValue * 3 + 3) >> 2;
    firstValue += (secondValue * 3 + 3) >> 3;
    secondValue -= secondTemporary;
    firstValue += firstTemporary;
    thirdValue += secondValue;
    fourthValue -= firstValue;

    *first = firstValue;
    *second = secondValue;
    *third = thirdValue;
    *fourth = fourthValue;
}

Void JxrForwardTransformMathApplyOddOddPre(
    PixelI* first,
    PixelI* second,
    PixelI* third,
    PixelI* fourth)
{
    PixelI firstValue = *first;
    PixelI secondValue = *second;
    PixelI thirdValue = *third;
    PixelI fourthValue = *fourth;
    PixelI firstTemporary;
    PixelI secondTemporary;

    fourthValue += firstValue;
    thirdValue -= secondValue;
    firstValue -= (firstTemporary = fourthValue >> 1);
    secondValue += (secondTemporary = thirdValue >> 1);
    firstValue += (secondValue * 3 + 4) >> 3;
    secondValue -= (firstValue * 3 + 2) >> 2;
    firstValue += (secondValue * 3 + 6) >> 3;
    secondValue -= secondTemporary;
    firstValue += firstTemporary;
    thirdValue += secondValue;
    fourthValue -= firstValue;

    *first = firstValue;
    *second = secondValue;
    *third = thirdValue;
    *fourth = fourthValue;
}

Void JxrForwardTransformMathApplyOdd(
    PixelI* first,
    PixelI* second,
    PixelI* third,
    PixelI* fourth)
{
    PixelI firstValue = *first;
    PixelI secondValue = *second;
    PixelI thirdValue = *third;
    PixelI fourthValue = *fourth;

    secondValue -= thirdValue;
    firstValue += fourthValue;
    thirdValue += (secondValue + 1) >> 1;
    fourthValue = ((firstValue + 1) >> 1) - fourthValue;
    JxrForwardTransformMathRotateThreeEighths(&firstValue, &secondValue);
    JxrForwardTransformMathRotateThreeEighths(&thirdValue, &fourthValue);
    fourthValue += secondValue >> 1;
    thirdValue -= (firstValue + 1) >> 1;
    secondValue -= fourthValue;
    firstValue += thirdValue;

    *first = firstValue;
    *second = secondValue;
    *third = thirdValue;
    *fourth = fourthValue;
}
