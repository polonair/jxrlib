#include "JxrImagePlaneDescriptorReader.h"

static Bool read_bits(SimpleBitIO* input, U32 count, U32* value)
{
    if (input == NULL || value == NULL) return FALSE;
    *value = getBit32_SB(input, count);
    return TRUE;
}

Bool JxrImagePlaneDescriptorReaderRead(SimpleBitIO* input, BITDEPTH_BITS bitDepth,
    JxrImagePlaneDescriptor* result)
{
    U32 value;
    if (input == NULL || result == NULL || !read_bits(input, 3, &value) ||
        value < Y_ONLY || value > NCOMPONENT) return FALSE;

    result->colorFormat = (COLORFORMAT)value;
    if (!read_bits(input, 1, &value)) return FALSE;
    result->scaledArithmetic = (Bool)value;
    if (!read_bits(input, 4, &value)) return FALSE;
    result->subband = (SUBBAND)value;
    result->channelCount = 0;
    result->hasChromaCenteringX = FALSE;
    result->hasChromaCenteringY = FALSE;
    result->chromaCenteringX = 0;
    result->chromaCenteringY = 0;
    result->hasSampleConversion = FALSE;
    result->mantissaOrShift = 0;
    result->exponentBias = 0;

    switch (result->colorFormat) {
    case Y_ONLY: result->channelCount = 1; break;
    case YUV_420:
        result->channelCount = 3;
        if (!read_bits(input, 1, &value) || !read_bits(input, 3, &value)) return FALSE;
        result->hasChromaCenteringX = TRUE; result->chromaCenteringX = (U8)value;
        if (!read_bits(input, 1, &value) || !read_bits(input, 3, &value)) return FALSE;
        result->hasChromaCenteringY = TRUE; result->chromaCenteringY = (U8)value;
        break;
    case YUV_422:
        result->channelCount = 3;
        if (!read_bits(input, 1, &value) || !read_bits(input, 3, &value)) return FALSE;
        result->hasChromaCenteringX = TRUE; result->chromaCenteringX = (U8)value;
        if (!read_bits(input, 4, &value)) return FALSE;
        break;
    case YUV_444:
        result->channelCount = 3;
        if (!read_bits(input, 4, &value) || !read_bits(input, 4, &value)) return FALSE;
        break;
    case NCOMPONENT:
        if (!read_bits(input, 4, &value)) return FALSE;
        result->channelCount = (size_t)value + 1;
        if (!read_bits(input, 4, &value)) return FALSE;
        break;
    case CMYK: result->channelCount = 4; break;
    default: return FALSE;
    }

    switch (bitDepth) {
    case BD_16: case BD_16S: case BD_32: case BD_32S:
        if (!read_bits(input, 8, &value)) return FALSE;
        result->hasSampleConversion = TRUE; result->mantissaOrShift = (U8)value;
        break;
    case BD_32F:
        if (!read_bits(input, 8, &value)) return FALSE;
        result->hasSampleConversion = TRUE; result->mantissaOrShift = (U8)value;
        if (!read_bits(input, 8, &value)) return FALSE;
        result->exponentBias = (I8)value;
        break;
    default: break;
    }
    return TRUE;
}
