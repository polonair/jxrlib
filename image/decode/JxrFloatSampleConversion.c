#include "JxrFloatSampleConversion.h"
#include <string.h>

typedef struct JxrRgbeComponent {
    U8 value;
    U8 exponent;
} JxrRgbeComponent;

static JxrRgbeComponent JxrFloatSampleConversionToRgbeComponent(PixelI value)
{
    JxrRgbeComponent result;
    if (value <= 0) {
        result.value = 0;
        result.exponent = 0;
    }
    else if ((value >> 7) > 1) {
        result.exponent = (U8)(value >> 7);
        result.value = (U8)((value & 0x7f) | 0x80);
    }
    else {
        result.exponent = 1;
        result.value = (U8)value;
    }
    return result;
}

JxrRgbeSample JxrFloatSampleConversionToRgbe(PixelI red, PixelI green, PixelI blue)
{
    JxrRgbeComponent redComponent = JxrFloatSampleConversionToRgbeComponent(red);
    JxrRgbeComponent greenComponent = JxrFloatSampleConversionToRgbeComponent(green);
    JxrRgbeComponent blueComponent = JxrFloatSampleConversionToRgbeComponent(blue);
    JxrRgbeSample result;
    U8 shift;

    result.red = redComponent.value;
    result.green = greenComponent.value;
    result.blue = blueComponent.value;
    result.exponent = redComponent.exponent;
    if (greenComponent.exponent > result.exponent) result.exponent = greenComponent.exponent;
    if (blueComponent.exponent > result.exponent) result.exponent = blueComponent.exponent;
    if (result.exponent > redComponent.exponent) {
        shift = result.exponent - redComponent.exponent;
        result.red = (U8)((((int)result.red) * 2 + 1) >> (shift + 1));
    }
    if (result.exponent > greenComponent.exponent) {
        shift = result.exponent - greenComponent.exponent;
        result.green = (U8)((((int)result.green) * 2 + 1) >> (shift + 1));
    }
    if (result.exponent > blueComponent.exponent) {
        shift = result.exponent - blueComponent.exponent;
        result.blue = (U8)((((int)result.blue) * 2 + 1) >> (shift + 1));
    }
    return result;
}

float JxrFloatSampleConversionToSingle(PixelI value, I8 exponentBias, U8 mantissaLength)
{
    I32 sign;
    I32 absoluteValue;
    I32 mantissa;
    I32 exponent;
    I32 normalizer = (1 << mantissaLength);
    U32 bits;
    float result;

    absoluteValue = (I32)value;
    sign = absoluteValue >> 31;
    absoluteValue = (absoluteValue ^ sign) - sign;
    exponent = (U32)absoluteValue >> mantissaLength;
    mantissa = (absoluteValue & (normalizer - 1)) | normalizer;
    if (exponent == 0) {
        mantissa ^= normalizer;
        exponent = 1;
    }
    exponent += 127 - exponentBias;
    while (mantissa < normalizer && exponent > 1 && mantissa > 0) {
        --exponent;
        mantissa <<= 1;
    }
    if (mantissa < normalizer) exponent = 0;
    else mantissa ^= normalizer;
    mantissa <<= 23 - mantissaLength;
    bits = (sign & 0x80000000) | (exponent << 23) | mantissa;
    memcpy(&result, &bits, sizeof(result));
    return result;
}

U16 JxrFloatSampleConversionToHalf(PixelI value)
{
    PixelI sign = value >> 31;
    value = ((value & 0x7fff) ^ sign) - sign;
    return (U16)value;
}

U32 JxrFloatSampleConversionSingleBits(float value)
{
    U32 bits;
    memcpy(&bits, &value, sizeof(bits));
    return bits;
}
