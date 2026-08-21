#ifndef JXR_ENCODER_SAMPLE_CONVERSION_H
#define JXR_ENCODER_SAMPLE_CONVERSION_H

#include "windowsmediaphoto.h"

PixelI JxrEncoderSampleConversionFromRgbe(PixelI component, PixelI exponent);
PixelI JxrEncoderSampleConversionFromHalf(PixelI halfValue);
PixelI JxrEncoderSampleConversionFromSingle(float value, I8 exponentBias,
    U8 mantissaLength);

#endif
