#include "JxrHighpassCbpState.h"
#include "JxrAdaptiveHuffman.h"

Void JxrHighpassCbpStateInit(JxrHighpassCbpState* state,
    CAdaptiveHuffman* patternHuffman, CAdaptiveHuffman* countHuffman,
    CCBPModel* predictionModel)
{
    state->patternHuffman = patternHuffman;
    state->countHuffman = countHuffman;
    state->predictionModel = predictionModel;
}

CAdaptiveHuffman* JxrHighpassCbpStateGetPatternHuffman(const JxrHighpassCbpState* state)
{
    return state->patternHuffman;
}

CAdaptiveHuffman* JxrHighpassCbpStateGetCountHuffman(const JxrHighpassCbpState* state)
{
    return state->countHuffman;
}

CCBPModel* JxrHighpassCbpStateGetPredictionModel(const JxrHighpassCbpState* state)
{
    return state->predictionModel;
}

Void JxrHighpassCbpStateAdapt(JxrHighpassCbpState* state)
{
    JxrAdaptiveHuffmanAdapt(state->patternHuffman);
    JxrAdaptiveHuffmanAdapt(state->countHuffman);
}
