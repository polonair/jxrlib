#ifndef JXR_FLOAT_SAMPLE_CONVERSION_H
#define JXR_FLOAT_SAMPLE_CONVERSION_H

#include "windowsmediaphoto.h"

typedef struct JxrRgbeSample {
    U8 red;
    U8 green;
    U8 blue;
    U8 exponent;
} JxrRgbeSample;

JxrRgbeSample JxrFloatSampleConversionToRgbe(PixelI red, PixelI green, PixelI blue);
float JxrFloatSampleConversionToSingle(PixelI value, I8 exponentBias, U8 mantissaLength);
U16 JxrFloatSampleConversionToHalf(PixelI value);
U32 JxrFloatSampleConversionSingleBits(float value);

#endif
