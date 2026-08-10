#ifndef JXR_ENTROPY_LEVEL_DECODER_H
#define JXR_ENTROPY_LEVEL_DECODER_H

#include "strcodec.h"
#include "JxrEntropyReader.h"

/* Decodes one non-zero coefficient magnitude from the adaptive Huffman stream. */
Int JxrEntropyLevelDecoderDecodeReader(CAdaptiveHuffman* state, JxrEntropyBitReader* input);
Int JxrEntropyLevelDecoderDecode(CAdaptiveHuffman* state, BitIOInfo* input);

#endif
