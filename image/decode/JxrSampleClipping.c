#include "JxrSampleClipping.h"

PixelI JxrSampleClippingClamp(PixelI value, PixelI lowerBound, PixelI upperBound)
{
    if (value < lowerBound) return lowerBound;
    if (value > upperBound) return upperBound;
    return value;
}

U8 JxrSampleClippingToByte(PixelI value)
{ return (U8)JxrSampleClippingClamp(value, 0, 255); }

U16 JxrSampleClippingToUInt16(PixelI value)
{ return (U16)JxrSampleClippingClamp(value, 0, 65535); }

I16 JxrSampleClippingToInt16(PixelI value)
{ return (I16)JxrSampleClippingClamp(value, -32768, 32767); }
