#include "JxrLpResidualDecoder.h"
#include "JxrEntropyReader.h"

static U32 JxrLpResidualDecoderRotateLeft(U32 value, Int bitCount)
{
    return (value << bitCount) | (value >> (32 - bitCount));
}

U32 JxrLpResidualDecoderReadBits(BitIOInfo* input, Int bitCount)
{
    if (bitCount > 14) return getBit32(input, bitCount);
    return JxrEntropyReaderRead(input, bitCount);
}

PixelI JxrLpResidualDecoderCombineNonZero(PixelI coefficient, U32 residual, Int bitCount)
{
    U32 mask = ((U32)1 << bitCount) - 1;
    U32 rotated = JxrLpResidualDecoderRotateLeft((U32)coefficient, bitCount);
    return (PixelI)((I32)((rotated ^ residual) - (rotated & mask)));
}

PixelI JxrLpResidualDecoderRefineNonZero(PixelI coefficient, BitIOInfo* input, Int bitCount)
{
    U32 residual = JxrLpResidualDecoderReadBits(input, bitCount);
    return JxrLpResidualDecoderCombineNonZero(coefficient, residual, bitCount);
}

PixelI JxrLpResidualDecoderCombineSignedMagnitude(PixelI coefficient, U32 residual, Int bitCount)
{
    U32 magnitude = (U32)(coefficient < 0 ? -coefficient : coefficient);
    PixelI refinedMagnitude = (PixelI)((magnitude << bitCount) + residual);
    return coefficient < 0 ? -refinedMagnitude : refinedMagnitude;
}

PixelI JxrLpResidualDecoderRefineSignedMagnitude(PixelI coefficient, BitIOInfo* input, Int bitCount)
{
    U32 residual = JxrLpResidualDecoderReadBits(input, bitCount);
    return JxrLpResidualDecoderCombineSignedMagnitude(coefficient, residual, bitCount);
}

PixelI JxrLpResidualDecoderReadSignedMagnitude(BitIOInfo* input, Int bitCount)
{
    PixelI magnitude = (PixelI)JxrLpResidualDecoderReadBits(input, bitCount);
    if (magnitude && JxrEntropyReaderReadFlag(input)) return -magnitude;
    return magnitude;
}

PixelI JxrLpResidualDecoderReadZeroCoefficient(BitIOInfo* input, Int bitCount)
{
    U32 encoded = JxrEntropyReaderPeek(input, bitCount + 1);
    PixelI coefficient = (PixelI)(((encoded >> 1) ^ (-(I32)(encoded & 1))) + (encoded & 1));
    JxrEntropyReaderConsume(input, bitCount + (coefficient != 0));
    return coefficient;
}

PixelI JxrLpResidualDecoderDecodeNormalCoefficient(PixelI coefficient, BitIOInfo* input, Int bitCount)
{
    if (coefficient) return JxrLpResidualDecoderRefineNonZero(coefficient, input, bitCount);
    return JxrLpResidualDecoderReadZeroCoefficient(input, bitCount);
}

PixelI JxrLpResidualDecoderDecodeChromaCoefficient(PixelI coefficient, BitIOInfo* input, Int bitCount)
{
    if (coefficient) return JxrLpResidualDecoderRefineSignedMagnitude(coefficient, input, bitCount);
    return JxrLpResidualDecoderReadSignedMagnitude(input, bitCount);
}
