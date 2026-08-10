#ifndef JXR_ADAPTIVE_HUFFMAN_H
#define JXR_ADAPTIVE_HUFFMAN_H

#include "strcodec.h"
#include "JxrEntropyReader.h"

Int JxrAdaptiveHuffmanDecodeReader(CAdaptiveHuffman* state, JxrEntropyBitReader* input);
Int JxrAdaptiveHuffmanDecodeShortTableReader(const short* table, JxrEntropyBitReader* input);
Int JxrAdaptiveHuffmanDecode(CAdaptiveHuffman* state, BitIOInfo* input);
Int JxrAdaptiveHuffmanDecodeShortTable(const short* table, BitIOInfo* input);
Void JxrAdaptiveHuffmanObserve(CAdaptiveHuffman* state, Int symbol);
Void JxrAdaptiveHuffmanAdapt(CAdaptiveHuffman* state);

#endif
