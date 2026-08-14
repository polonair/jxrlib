#include "JxrInverseTransformMath.h"
#include "JxrTransformMath.h"

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

Void JxrInverseTransformMathApplyDcCompensation(
    PixelI* topLeft,
    PixelI* topRight,
    PixelI* bottomLeft,
    PixelI* bottomRight,
    Int directCurrent)
{
    Int halfDirectCurrent = directCurrent >> 1;

    *topLeft -= halfDirectCurrent;
    *bottomRight -= halfDirectCurrent;
    *topRight += halfDirectCurrent;
    *bottomLeft += halfDirectCurrent;
}

Int JxrInverseTransformMathClipDcWithAlternate(
    Int directCurrent,
    Int alternateCurrent)
{
    if (directCurrent > 0 && alternateCurrent > 0) {
        return directCurrent < alternateCurrent ? directCurrent : alternateCurrent;
    }
    if (directCurrent < 0 && alternateCurrent < 0) {
        return directCurrent > alternateCurrent ? directCurrent : alternateCurrent;
    }

    return 0;
}

Int JxrInverseTransformMathApplyConditionalDcCompensation(
    PixelI* topLeft,
    PixelI* topRight,
    PixelI* bottomLeft,
    PixelI* bottomRight,
    Int directCurrent,
    Int highPassQuantizer,
    Bool highPassAbsent)
{
    Int alternateCurrent;

    if (!JxrInverseTransformMathShouldCompensateDc(
        directCurrent,
        highPassQuantizer,
        highPassAbsent)) {
        return directCurrent;
    }

    alternateCurrent = (*topLeft - *bottomLeft - *topRight + *bottomRight) >> 1;
    directCurrent = JxrInverseTransformMathClipDcWithAlternate(
        directCurrent,
        alternateCurrent);
    JxrInverseTransformMathApplyDcCompensation(
        topLeft,
        topRight,
        bottomLeft,
        bottomRight,
        directCurrent);
    return directCurrent;
}

Void JxrInverseTransformMathApplyPost4(
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

    JxrInverseTransformMathRotateHalf(&thirdValue, &fourthValue);

    fourthValue += (firstValue + 1) >> 1;
    thirdValue += (secondValue + 1) >> 1;
    firstValue -= fourthValue - ((fourthValue * 3 + 16) >> 5);
    secondValue -= thirdValue - ((thirdValue * 3 + 16) >> 5);
    fourthValue += (firstValue * 3 + 8) >> 4;
    thirdValue += (secondValue * 3 + 8) >> 4;
    firstValue += (fourthValue * 3 + 16) >> 5;
    secondValue += (thirdValue * 3 + 16) >> 5;

    *first = firstValue;
    *second = secondValue;
    *third = thirdValue;
    *fourth = fourthValue;
}

static Void JxrInverseTransformMathApplyAlternatePost4Edge(
    PixelI* first,
    PixelI* fourth)
{
    PixelI firstValue = *first;
    PixelI fourthValue = *fourth;

    firstValue += fourthValue;
    fourthValue = (firstValue >> 1) - fourthValue;
    firstValue += (fourthValue * 3) >> 3;
    fourthValue += (firstValue * 3) >> 4;
    fourthValue += firstValue >> 7;
    fourthValue -= firstValue >> 10;
    firstValue += (fourthValue * 3 + 4) >> 3;
    fourthValue -= firstValue >> 1;
    firstValue += fourthValue;

    *first = firstValue;
    *fourth = -fourthValue;
}

Void JxrInverseTransformMathApplyAlternatePost4(
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

    JxrInverseTransformMathApplyAlternatePost4Edge(&firstValue, &fourthValue);
    JxrInverseTransformMathApplyAlternatePost4Edge(&secondValue, &thirdValue);
    JxrInverseTransformMathRotateHalf(&thirdValue, &fourthValue);
    fourthValue += (firstValue + 1) >> 1;
    thirdValue += (secondValue + 1) >> 1;
    firstValue -= fourthValue;
    secondValue -= thirdValue;

    *first = firstValue;
    *second = secondValue;
    *third = thirdValue;
    *fourth = fourthValue;
}

Void JxrInverseTransformMathApplyHadamardScale4(
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
    firstValue += (fourthValue * 3 + 4) >> 3;
    fourthValue -= secondValue >> 1;
    thirdValue = ((firstValue - secondValue) >> 1) - thirdValue;

    *third = fourthValue;
    *fourth = thirdValue;
    *first = firstValue - thirdValue;
    *second = secondValue + fourthValue;
}

Void JxrInverseTransformMathApplyHadamardScale2(
    PixelI* first,
    PixelI* second)
{
    PixelI firstValue = *first;
    PixelI secondValue = *second;

    firstValue += secondValue;
    secondValue = (firstValue >> 1) - secondValue;
    firstValue += (secondValue * 3) >> 3;
    secondValue += (firstValue * 3) >> 4;

    *first = firstValue;
    *second = secondValue;
}

Void JxrInverseTransformMathApplyAlternateHadamardScale2(
    PixelI* first,
    PixelI* second)
{
    JxrInverseTransformMathApplyHadamardScale2(first, second);
    *second += *first >> 7;
    *second -= *first >> 10;
}

static Void JxrInverseTransformMathApplyOddOddCore(
    PixelI* first,
    PixelI* second,
    PixelI* third,
    PixelI* fourth,
    Int firstRotationRounding,
    Int secondRotationRounding,
    Bool negateMiddleOutputs)
{
    PixelI firstValue = *first;
    PixelI secondValue = *second;
    PixelI thirdValue = *third;
    PixelI fourthValue = *fourth;
    PixelI firstHalf;
    PixelI secondHalf;

    fourthValue += firstValue;
    thirdValue -= secondValue;
    firstHalf = fourthValue >> 1;
    secondHalf = thirdValue >> 1;
    firstValue -= firstHalf;
    secondValue += secondHalf;

    firstValue -= (secondValue * 3 + firstRotationRounding) >> 3;
    secondValue += (firstValue * 3 + secondRotationRounding) >> 2;
    firstValue -= (secondValue * 3 + 4) >> 3;

    secondValue -= secondHalf;
    firstValue += firstHalf;
    thirdValue += secondValue;
    fourthValue -= firstValue;

    *first = firstValue;
    *second = negateMiddleOutputs ? -secondValue : secondValue;
    *third = negateMiddleOutputs ? -thirdValue : thirdValue;
    *fourth = fourthValue;
}

Void JxrInverseTransformMathApplyOddOdd(
    PixelI* first,
    PixelI* second,
    PixelI* third,
    PixelI* fourth)
{
    JxrInverseTransformMathApplyOddOddCore(
        first, second, third, fourth, 3, 3, TRUE);
}

Void JxrInverseTransformMathApplyOddOddPost(
    PixelI* first,
    PixelI* second,
    PixelI* third,
    PixelI* fourth)
{
    JxrInverseTransformMathApplyOddOddCore(
        first, second, third, fourth, 6, 2, FALSE);
}

Void JxrInverseTransformMathApplyOdd(
    PixelI* first,
    PixelI* second,
    PixelI* third,
    PixelI* fourth)
{
    PixelI firstValue = *first;
    PixelI secondValue = *second;
    PixelI thirdValue = *third;
    PixelI fourthValue = *fourth;

    secondValue += fourthValue;
    firstValue -= thirdValue;
    fourthValue -= secondValue >> 1;
    thirdValue += (firstValue + 1) >> 1;

    JxrInverseTransformMathRotateThreeEighths(&firstValue, &secondValue);
    JxrInverseTransformMathRotateThreeEighths(&thirdValue, &fourthValue);

    thirdValue -= (secondValue + 1) >> 1;
    fourthValue = ((firstValue + 1) >> 1) - fourthValue;
    secondValue += thirdValue;
    firstValue -= fourthValue;

    *first = firstValue;
    *second = secondValue;
    *third = thirdValue;
    *fourth = fourthValue;
}

Void JxrInverseTransformMathApplyScaledDct2x2Down(
    PixelI* first,
    PixelI* second,
    PixelI* third,
    PixelI* fourth)
{
    JxrTransformMathApplyDct2x2Down(first, second, third, fourth);
    *first *= 2;
    *second *= 2;
    *third *= 2;
    *fourth *= 2;
}

Void JxrInverseTransformMathApplyPost2(
    PixelI* first,
    PixelI* second)
{
    PixelI firstValue = *first;
    PixelI secondValue = *second;

    secondValue += (firstValue + 4) >> 3;
    firstValue += (secondValue + 2) >> 2;
    secondValue += (firstValue + 4) >> 3;

    *first = firstValue;
    *second = secondValue;
}

Void JxrInverseTransformMathApplyAlternatePost2(
    PixelI* first,
    PixelI* second)
{
    PixelI firstValue = *first;
    PixelI secondValue = *second;

    secondValue += (firstValue + 2) >> 2;
    firstValue += (secondValue + 1) >> 1;
    firstValue += secondValue >> 5;
    firstValue += secondValue >> 9;
    firstValue += secondValue >> 13;
    secondValue += (firstValue + 2) >> 2;

    *first = firstValue;
    *second = secondValue;
}
