#ifndef JXR_HUFFMAN_DECODER_H
#define JXR_HUFFMAN_DECODER_H

#include "strcodec.h"

enum {
    JXR_HUFFMAN_ROOT_BITS = 5,
    JXR_HUFFMAN_ENCODED_LENGTH_BITS = 3,
    JXR_HUFFMAN_BRANCH_OFFSET = 0x8000
};

typedef struct JxrHuffmanTable {
    const short* entries;
} JxrHuffmanTable;

JxrHuffmanTable JxrHuffmanTableCreate(const short* entries);
Int JxrHuffmanTableGetEntry(const JxrHuffmanTable* table, UInt index);
Int JxrHuffmanDecoderDecodeSymbol(const JxrHuffmanTable* table, BitIOInfo* input);
Int JxrHuffmanDecoderDecodeShortSymbol(const JxrHuffmanTable* table, BitIOInfo* input);

#endif
