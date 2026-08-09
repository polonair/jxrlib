#include "JxrAdaptiveHuffman.h"
#include "JxrEntropyReader.h"
#include "JxrHuffmanDecoder.h"

Int JxrAdaptiveHuffmanDecode(CAdaptiveHuffman* state, BitIOInfo* input)
{
    JxrHuffmanTable table = JxrHuffmanTableCreate(state->m_hufDecTable);
    Int symbol = JxrHuffmanDecoderDecodeSymbol(&table, input);
    JxrAdaptiveHuffmanObserve(state, symbol);
    return symbol;
}

Int JxrAdaptiveHuffmanDecodeShortTable(const short* table, BitIOInfo* input)
{
    JxrHuffmanTable huffmanTable = JxrHuffmanTableCreate(table);
    return JxrHuffmanDecoderDecodeShortSymbol(&huffmanTable, input);
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
