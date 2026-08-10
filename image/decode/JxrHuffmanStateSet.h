#ifndef JXR_HUFFMAN_STATE_SET_H
#define JXR_HUFFMAN_STATE_SET_H

#include "strcodec.h"

/* Named collection of adaptive Huffman states addressed by entropy context index. */
typedef struct JxrHuffmanStateSet {
    CAdaptiveHuffman** nativeStates;
} JxrHuffmanStateSet;

Void JxrHuffmanStateSetInit(JxrHuffmanStateSet* state, CAdaptiveHuffman** nativeStates);
CAdaptiveHuffman* JxrHuffmanStateSetGet(const JxrHuffmanStateSet* state, Int index);
Void JxrHuffmanStateSetObserve(JxrHuffmanStateSet* state, Int index, Int symbol);
Void JxrHuffmanStateSetAdapt(JxrHuffmanStateSet* state, Int index);

#endif
