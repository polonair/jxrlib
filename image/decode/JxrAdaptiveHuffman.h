#ifndef JXR_ADAPTIVE_HUFFMAN_H
#define JXR_ADAPTIVE_HUFFMAN_H

#include "strcodec.h"

Int JxrAdaptiveHuffmanDecode(CAdaptiveHuffman* state, BitIOInfo* input);
Int JxrAdaptiveHuffmanDecodeShortTable(const short* table, BitIOInfo* input);
Void JxrAdaptiveHuffmanObserve(CAdaptiveHuffman* state, Int symbol);
Void JxrAdaptiveHuffmanAdapt(CAdaptiveHuffman* state);

#endif
