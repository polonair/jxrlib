#ifndef JXR_SAMPLE_CLIPPING_H
#define JXR_SAMPLE_CLIPPING_H

#include "windowsmediaphoto.h"

PixelI JxrSampleClippingClamp(PixelI value, PixelI lowerBound, PixelI upperBound);
U8 JxrSampleClippingToByte(PixelI value);
U16 JxrSampleClippingToUInt16(PixelI value);
I16 JxrSampleClippingToInt16(PixelI value);

#endif
