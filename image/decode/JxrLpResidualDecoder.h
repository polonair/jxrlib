#ifndef JXR_LP_RESIDUAL_DECODER_H
#define JXR_LP_RESIDUAL_DECODER_H

#include "strcodec.h"
#include "JxrEntropyReader.h"

U32 JxrLpResidualDecoderReadBitsReader(JxrEntropyBitReader* input, Int bitCount);
PixelI JxrLpResidualDecoderRefineNonZeroReader(PixelI coefficient, JxrEntropyBitReader* input, Int bitCount);
PixelI JxrLpResidualDecoderRefineSignedMagnitudeReader(PixelI coefficient, JxrEntropyBitReader* input, Int bitCount);
PixelI JxrLpResidualDecoderReadSignedMagnitudeReader(JxrEntropyBitReader* input, Int bitCount);
PixelI JxrLpResidualDecoderReadZeroCoefficientReader(JxrEntropyBitReader* input, Int bitCount);
PixelI JxrLpResidualDecoderDecodeNormalCoefficientReader(PixelI coefficient, JxrEntropyBitReader* input, Int bitCount);
PixelI JxrLpResidualDecoderDecodeChromaCoefficientReader(PixelI coefficient, JxrEntropyBitReader* input, Int bitCount);
U32 JxrLpResidualDecoderReadBits(BitIOInfo* input, Int bitCount);
PixelI JxrLpResidualDecoderCombineNonZero(PixelI coefficient, U32 residual, Int bitCount);
PixelI JxrLpResidualDecoderRefineNonZero(PixelI coefficient, BitIOInfo* input, Int bitCount);
PixelI JxrLpResidualDecoderCombineSignedMagnitude(PixelI coefficient, U32 residual, Int bitCount);
PixelI JxrLpResidualDecoderRefineSignedMagnitude(PixelI coefficient, BitIOInfo* input, Int bitCount);
PixelI JxrLpResidualDecoderReadSignedMagnitude(BitIOInfo* input, Int bitCount);
PixelI JxrLpResidualDecoderReadZeroCoefficient(BitIOInfo* input, Int bitCount);
PixelI JxrLpResidualDecoderDecodeNormalCoefficient(PixelI coefficient, BitIOInfo* input, Int bitCount);
PixelI JxrLpResidualDecoderDecodeChromaCoefficient(PixelI coefficient, BitIOInfo* input, Int bitCount);

#endif
