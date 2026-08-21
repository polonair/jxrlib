#include "JxrEncoderSampleConversion.h"
#include <string.h>

PixelI JxrEncoderSampleConversionFromRgbe(PixelI component, PixelI exponent)
{
    PixelI result = 0;
    PixelI appendBit = 1;

    if (exponent == 0)
        return 0;

    --exponent;
    while ((component & 0x80) == 0 && exponent > 0) {
        component = (component << 1) + appendBit;
        appendBit = 0;
        --exponent;
    }

    if (exponent == 0)
        result = component;
    else {
        ++exponent;
        result = (component & 0x7f) + (exponent << 7);
    }
    return result;
}

PixelI JxrEncoderSampleConversionFromHalf(PixelI halfValue)
{
    PixelI sign = halfValue >> 31;
    return ((halfValue & 0x7fff) ^ sign) - sign;
}

PixelI JxrEncoderSampleConversionFromSingle(float value, I8 exponentBias,
    U8 mantissaLength)
{
    I32 bits;
    PixelI result;
    PixelI exponent;
    PixelI adjustedExponent;
    PixelI mantissa;
    PixelI sign;

    if (value == 0)
        return 0;

    memcpy(&bits, &value, sizeof(bits));
    exponent = (bits >> 23) & 0x000000ff;
    mantissa = (bits & 0x007fffff) | 0x800000;
    if (exponent == 0) {
        mantissa ^= 0x800000;
        ++exponent;
    }

    adjustedExponent = exponent - 127 + exponentBias;
    if (adjustedExponent <= 1) {
        if (adjustedExponent < 1)
            mantissa >>= 1 - adjustedExponent;
        adjustedExponent = 1;
        if ((mantissa & 0x800000) == 0)
            adjustedExponent = 0;
    }
    mantissa &= 0x007fffff;
    result = (adjustedExponent << mantissaLength) +
        ((mantissa + (1 << (23 - mantissaLength - 1))) >>
        (23 - mantissaLength));
    sign = ((PixelI)bits) >> 31;
    return (result ^ sign) - sign;
}
