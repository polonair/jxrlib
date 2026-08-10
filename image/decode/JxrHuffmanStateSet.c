#include "JxrHuffmanStateSet.h"
#include "JxrAdaptiveHuffman.h"

Void JxrHuffmanStateSetInit(JxrHuffmanStateSet* state, CAdaptiveHuffman** nativeStates)
{
    state->nativeStates = nativeStates;
}

CAdaptiveHuffman* JxrHuffmanStateSetGet(const JxrHuffmanStateSet* state, Int index)
{
    return state->nativeStates[index];
}

Void JxrHuffmanStateSetObserve(JxrHuffmanStateSet* state, Int index, Int symbol)
{
    JxrAdaptiveHuffmanObserve(JxrHuffmanStateSetGet(state, index), symbol);
}

Void JxrHuffmanStateSetAdapt(JxrHuffmanStateSet* state, Int index)
{
    JxrAdaptiveHuffmanAdapt(JxrHuffmanStateSetGet(state, index));
}
