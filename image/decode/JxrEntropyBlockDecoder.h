#ifndef JXR_ENTROPY_BLOCK_DECODER_H
#define JXR_ENTROPY_BLOCK_DECODER_H

#include "strcodec.h"
#include "JxrEntropyReader.h"

Int JxrEntropyBlockDecoderDecodeRunReader(Int maximumRun, CAdaptiveHuffman* state, JxrEntropyBitReader* input);
Int JxrEntropyBlockDecoderDecodeFirstSymbolReader(CAdaptiveHuffman* state, JxrEntropyBitReader* input);
Int JxrEntropyBlockDecoderDecodeNextSymbolReader(Int coefficientPosition,
    CAdaptiveHuffman* state, JxrEntropyBitReader* input);
Int JxrEntropyBlockDecoderDecodeLowpassBlockReader(Bool isChroma, Int* runLevelPairs,
    CAdaptiveHuffman** huffmanStates, Int contextOffset, JxrEntropyBitReader* input, Int startPosition);
Int JxrEntropyBlockDecoderDecodeRun(Int maximumRun, CAdaptiveHuffman* state, BitIOInfo* input);
Int JxrEntropyBlockDecoderDecodeFirstSymbol(CAdaptiveHuffman* state, BitIOInfo* input);
Int JxrEntropyBlockDecoderDecodeNextSymbol(Int coefficientPosition, CAdaptiveHuffman* state, BitIOInfo* input);
Int JxrEntropyBlockDecoderDecodeLowpassBlock(Bool isChroma, Int* runLevelPairs,
    CAdaptiveHuffman** huffmanStates, Int contextOffset, BitIOInfo* input, Int startPosition);

#endif
