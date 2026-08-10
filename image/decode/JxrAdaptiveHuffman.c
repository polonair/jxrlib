#include "JxrAdaptiveHuffman.h"
#include "JxrEntropyReader.h"
#include "JxrHuffmanDecoder.h"

Int JxrAdaptiveHuffmanDecodeReader(CAdaptiveHuffman* state, JxrEntropyBitReader* input)
{
    JxrHuffmanTable table = JxrHuffmanTableCreate(state->m_hufDecTable);
    Int symbol = JxrHuffmanDecoderDecodeSymbolReader(&table, input);
    JxrAdaptiveHuffmanObserve(state, symbol);
    return symbol;
}

Int JxrAdaptiveHuffmanDecodeShortTableReader(const short* table, JxrEntropyBitReader* input)
{
    JxrHuffmanTable huffmanTable = JxrHuffmanTableCreate(table);
    return JxrHuffmanDecoderDecodeShortSymbolReader(&huffmanTable, input);
}

Int JxrAdaptiveHuffmanDecode(CAdaptiveHuffman* state, BitIOInfo* input)
{
    JxrEntropyBitReader reader;
    JxrEntropyBitReaderInit(&reader, input);
    return JxrAdaptiveHuffmanDecodeReader(state, &reader);
}

Int JxrAdaptiveHuffmanDecodeShortTable(const short* table, BitIOInfo* input)
{
    JxrEntropyBitReader reader;
    JxrEntropyBitReaderInit(&reader, input);
    return JxrAdaptiveHuffmanDecodeShortTableReader(table, &reader);
}

Void JxrAdaptiveHuffmanObserve(CAdaptiveHuffman* state, Int symbol)
{
    state->m_iDiscriminant += state->m_pDelta[symbol];
    if (state->m_pDelta1 != NULL)
        state->m_iDiscriminant1 += state->m_pDelta1[symbol];
}

Void JxrAdaptiveHuffmanAdapt(CAdaptiveHuffman* state)
{
    AdaptDiscriminant(state);
}
