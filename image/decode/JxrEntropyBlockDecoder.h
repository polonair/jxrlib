#ifndef JXR_ENTROPY_BLOCK_DECODER_H
#define JXR_ENTROPY_BLOCK_DECODER_H

#include "strcodec.h"

Int JxrEntropyBlockDecoderDecodeRun(Int maximumRun, CAdaptiveHuffman* state, BitIOInfo* input);
Int JxrEntropyBlockDecoderDecodeFirstSymbol(CAdaptiveHuffman* state, BitIOInfo* input);
Int JxrEntropyBlockDecoderDecodeNextSymbol(Int coefficientPosition, CAdaptiveHuffman* state, BitIOInfo* input);
Int JxrEntropyBlockDecoderDecodeLowpassBlock(Bool isChroma, Int* runLevelPairs,
    CAdaptiveHuffman** huffmanStates, Int contextOffset, BitIOInfo* input, Int startPosition);

#endif
