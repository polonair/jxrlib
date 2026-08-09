#include "JxrAdaptiveHuffman.h"
#include "JxrEntropyReader.h"
#include "decode.h"

Int JxrAdaptiveHuffmanDecode(CAdaptiveHuffman* state, BitIOInfo* input)
{
    Int symbol = getHuff(state->m_hufDecTable, input);
    JxrAdaptiveHuffmanObserve(state, symbol);
    return symbol;
}

Int JxrAdaptiveHuffmanDecodeShortTable(const short* table, BitIOInfo* input)
{
    Int encoded = table[JxrEntropyReaderPeek(input, HUFFMAN_DECODE_ROOT_BITS)];
    assert(encoded >= 0);
    JxrEntropyReaderConsume(input, encoded & ((1 << HUFFMAN_DECODE_ROOT_BITS_LOG) - 1));
    return encoded >> HUFFMAN_DECODE_ROOT_BITS_LOG;
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
