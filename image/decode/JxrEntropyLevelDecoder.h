#ifndef JXR_ENTROPY_LEVEL_DECODER_H
#define JXR_ENTROPY_LEVEL_DECODER_H

#include "strcodec.h"

/* Decodes one non-zero coefficient magnitude from the adaptive Huffman stream. */
Int JxrEntropyLevelDecoderDecode(CAdaptiveHuffman* state, BitIOInfo* input);

#endif
