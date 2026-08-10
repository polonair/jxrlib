#include "JxrLpResidualDecoder.h"
#include "JxrEntropyReader.h"
#include "JxrBitMath.h"

U32 JxrLpResidualDecoderReadBitsReader(JxrEntropyBitReader* input, Int bitCount)
{
    if (bitCount > 14) return JxrEntropyBitReaderReadLong(input, bitCount);
    return JxrEntropyBitReaderRead(input, bitCount);
}

PixelI JxrLpResidualDecoderCombineNonZero(PixelI coefficient, U32 residual, Int bitCount)
{
    U32 mask = JxrBitMathLowMask32((U32)bitCount);
    U32 rotated = JxrBitMathRotateLeft32((U32)coefficient, (U32)bitCount);
    return (PixelI)((I32)((rotated ^ residual) - (rotated & mask)));
}

PixelI JxrLpResidualDecoderRefineNonZeroReader(PixelI coefficient, JxrEntropyBitReader* input, Int bitCount)
{
    U32 residual = JxrLpResidualDecoderReadBitsReader(input, bitCount);
    return JxrLpResidualDecoderCombineNonZero(coefficient, residual, bitCount);
}

PixelI JxrLpResidualDecoderCombineSignedMagnitude(PixelI coefficient, U32 residual, Int bitCount)
{
    U32 magnitude = (U32)(coefficient < 0 ? -coefficient : coefficient);
    PixelI refinedMagnitude = (PixelI)((magnitude << bitCount) + residual);
    return coefficient < 0 ? -refinedMagnitude : refinedMagnitude;
}

PixelI JxrLpResidualDecoderRefineSignedMagnitudeReader(PixelI coefficient, JxrEntropyBitReader* input, Int bitCount)
{
    U32 residual = JxrLpResidualDecoderReadBitsReader(input, bitCount);
    return JxrLpResidualDecoderCombineSignedMagnitude(coefficient, residual, bitCount);
}

PixelI JxrLpResidualDecoderReadSignedMagnitudeReader(JxrEntropyBitReader* input, Int bitCount)
{
    PixelI magnitude = (PixelI)JxrLpResidualDecoderReadBitsReader(input, bitCount);
    if (magnitude && JxrEntropyBitReaderReadFlag(input)) return -magnitude;
    return magnitude;
}

PixelI JxrLpResidualDecoderReadZeroCoefficientReader(JxrEntropyBitReader* input, Int bitCount)
{
    U32 encoded = JxrEntropyBitReaderPeek(input, bitCount + 1);
    PixelI coefficient = (PixelI)JxrEntropyBitReaderDecodeSignedResidualValue(encoded);
    JxrEntropyBitReaderConsume(input, bitCount + (coefficient != 0));
    return coefficient;
}

PixelI JxrLpResidualDecoderDecodeNormalCoefficientReader(PixelI coefficient, JxrEntropyBitReader* input, Int bitCount)
{
    if (coefficient) return JxrLpResidualDecoderRefineNonZeroReader(coefficient, input, bitCount);
    return JxrLpResidualDecoderReadZeroCoefficientReader(input, bitCount);
}

PixelI JxrLpResidualDecoderDecodeChromaCoefficientReader(PixelI coefficient, JxrEntropyBitReader* input, Int bitCount)
{
    if (coefficient) return JxrLpResidualDecoderRefineSignedMagnitudeReader(coefficient, input, bitCount);
    return JxrLpResidualDecoderReadSignedMagnitudeReader(input, bitCount);
}

U32 JxrLpResidualDecoderReadBits(BitIOInfo* input, Int bitCount)
{
    JxrEntropyBitReader reader;
    JxrEntropyBitReaderInit(&reader, input);
    return JxrLpResidualDecoderReadBitsReader(&reader, bitCount);
}

PixelI JxrLpResidualDecoderRefineNonZero(PixelI coefficient, BitIOInfo* input, Int bitCount)
{
    JxrEntropyBitReader reader;
    JxrEntropyBitReaderInit(&reader, input);
    return JxrLpResidualDecoderRefineNonZeroReader(coefficient, &reader, bitCount);
}

PixelI JxrLpResidualDecoderRefineSignedMagnitude(PixelI coefficient, BitIOInfo* input, Int bitCount)
{
    JxrEntropyBitReader reader;
    JxrEntropyBitReaderInit(&reader, input);
    return JxrLpResidualDecoderRefineSignedMagnitudeReader(coefficient, &reader, bitCount);
}

PixelI JxrLpResidualDecoderReadSignedMagnitude(BitIOInfo* input, Int bitCount)
{
    JxrEntropyBitReader reader;
    JxrEntropyBitReaderInit(&reader, input);
    return JxrLpResidualDecoderReadSignedMagnitudeReader(&reader, bitCount);
}

PixelI JxrLpResidualDecoderReadZeroCoefficient(BitIOInfo* input, Int bitCount)
{
    JxrEntropyBitReader reader;
    JxrEntropyBitReaderInit(&reader, input);
    return JxrLpResidualDecoderReadZeroCoefficientReader(&reader, bitCount);
}

PixelI JxrLpResidualDecoderDecodeNormalCoefficient(PixelI coefficient, BitIOInfo* input, Int bitCount)
{
    JxrEntropyBitReader reader;
    JxrEntropyBitReaderInit(&reader, input);
    return JxrLpResidualDecoderDecodeNormalCoefficientReader(coefficient, &reader, bitCount);
}

PixelI JxrLpResidualDecoderDecodeChromaCoefficient(PixelI coefficient, BitIOInfo* input, Int bitCount)
{
    JxrEntropyBitReader reader;
    JxrEntropyBitReaderInit(&reader, input);
    return JxrLpResidualDecoderDecodeChromaCoefficientReader(coefficient, &reader, bitCount);
}
