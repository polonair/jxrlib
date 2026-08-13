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
