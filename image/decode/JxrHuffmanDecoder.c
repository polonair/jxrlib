#include "JxrHuffmanDecoder.h"
#include "JxrEntropyReader.h"

JxrHuffmanTable JxrHuffmanTableCreate(const short* entries)
{
    JxrHuffmanTable table;
    table.entries = entries;
    return table;
}

Int JxrHuffmanTableGetEntry(const JxrHuffmanTable* table, UInt index)
{
    return table->entries[index];
}

Int JxrHuffmanDecoderDecodeSymbol(const JxrHuffmanTable* table, BitIOInfo* input)
{
    Int encodedEntry;
    Int branchEntry;
    Int symbol;
    UInt branchIndex;

    encodedEntry = JxrHuffmanTableGetEntry(table,
        JxrEntropyReaderPeek(input, JXR_HUFFMAN_ROOT_BITS));
    JxrEntropyReaderConsume(input, encodedEntry < 0 ? JXR_HUFFMAN_ROOT_BITS :
        encodedEntry & ((1 << JXR_HUFFMAN_ENCODED_LENGTH_BITS) - 1));
    symbol = encodedEntry >> JXR_HUFFMAN_ENCODED_LENGTH_BITS;

    branchEntry = encodedEntry;
    while (symbol < 0) {
        branchIndex = (UInt)(branchEntry + JXR_HUFFMAN_BRANCH_OFFSET) +
            JxrEntropyReaderRead(input, 1);
        branchEntry = JxrHuffmanTableGetEntry(table, branchIndex);
        symbol = branchEntry;
    }
    return symbol;
}

Int JxrHuffmanDecoderDecodeShortSymbol(const JxrHuffmanTable* table, BitIOInfo* input)
{
    Int encodedEntry = JxrHuffmanTableGetEntry(table,
        JxrEntropyReaderPeek(input, JXR_HUFFMAN_ROOT_BITS));
    assert(encodedEntry >= 0);
    JxrEntropyReaderConsume(input,
        encodedEntry & ((1 << JXR_HUFFMAN_ENCODED_LENGTH_BITS) - 1));
    return encodedEntry >> JXR_HUFFMAN_ENCODED_LENGTH_BITS;
}
