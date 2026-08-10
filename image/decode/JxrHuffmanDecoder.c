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

Int JxrHuffmanDecoderDecodeSymbolReader(const JxrHuffmanTable* table, JxrEntropyBitReader* input)
{
    Int encodedEntry;
    Int branchEntry;
    Int symbol;
    UInt branchIndex;

    encodedEntry = JxrHuffmanTableGetEntry(table,
        JxrEntropyBitReaderPeek(input, JXR_HUFFMAN_ROOT_BITS));
    JxrEntropyBitReaderConsume(input, encodedEntry < 0 ? JXR_HUFFMAN_ROOT_BITS :
        encodedEntry & ((1 << JXR_HUFFMAN_ENCODED_LENGTH_BITS) - 1));
    symbol = encodedEntry >> JXR_HUFFMAN_ENCODED_LENGTH_BITS;

    branchEntry = encodedEntry;
    while (symbol < 0) {
        branchIndex = (UInt)(branchEntry + JXR_HUFFMAN_BRANCH_OFFSET) +
            JxrEntropyBitReaderRead(input, 1);
        branchEntry = JxrHuffmanTableGetEntry(table, branchIndex);
        symbol = branchEntry;
    }
    return symbol;
}

Int JxrHuffmanDecoderDecodeShortSymbolReader(const JxrHuffmanTable* table, JxrEntropyBitReader* input)
{
    Int encodedEntry = JxrHuffmanTableGetEntry(table,
        JxrEntropyBitReaderPeek(input, JXR_HUFFMAN_ROOT_BITS));
    assert(encodedEntry >= 0);
    JxrEntropyBitReaderConsume(input,
        encodedEntry & ((1 << JXR_HUFFMAN_ENCODED_LENGTH_BITS) - 1));
    return encodedEntry >> JXR_HUFFMAN_ENCODED_LENGTH_BITS;
}

Int JxrHuffmanDecoderDecodeSymbol(const JxrHuffmanTable* table, BitIOInfo* input)
{
    JxrEntropyBitReader reader;
    JxrEntropyBitReaderInit(&reader, input);
    return JxrHuffmanDecoderDecodeSymbolReader(table, &reader);
}

Int JxrHuffmanDecoderDecodeShortSymbol(const JxrHuffmanTable* table, BitIOInfo* input)
{
    JxrEntropyBitReader reader;
    JxrEntropyBitReaderInit(&reader, input);
    return JxrHuffmanDecoderDecodeShortSymbolReader(table, &reader);
}
