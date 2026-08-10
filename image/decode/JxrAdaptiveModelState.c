#include "JxrAdaptiveModelState.h"

Void JxrAdaptiveModelStateInit(JxrAdaptiveModelState* state, CAdaptiveModel* nativeModel)
{
    state->nativeModel = nativeModel;
}

Int JxrAdaptiveModelStateGetFlcBits(const JxrAdaptiveModelState* state, Int channel)
{
    return state->nativeModel->m_iFlcBits[channel];
}

Void JxrAdaptiveModelStateUpdateForMacroblock(JxrAdaptiveModelState* state,
    COLORFORMAT colorFormat, Int channelCount, Int* laplacianMean)
{
    UpdateModelMB(colorFormat, channelCount, laplacianMean, state->nativeModel);
}
