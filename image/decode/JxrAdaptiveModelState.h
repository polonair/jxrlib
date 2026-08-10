#ifndef JXR_ADAPTIVE_MODEL_STATE_H
#define JXR_ADAPTIVE_MODEL_STATE_H

#include "strcodec.h"

/* Explicit adaptive-model operations used by DC, LP, and HP subbands. */
typedef struct JxrAdaptiveModelState {
    CAdaptiveModel* nativeModel;
} JxrAdaptiveModelState;

Void JxrAdaptiveModelStateInit(JxrAdaptiveModelState* state, CAdaptiveModel* nativeModel);
Int JxrAdaptiveModelStateGetFlcBits(const JxrAdaptiveModelState* state, Int channel);
Void JxrAdaptiveModelStateUpdateForMacroblock(JxrAdaptiveModelState* state,
    COLORFORMAT colorFormat, Int channelCount, Int* laplacianMean);

#endif
