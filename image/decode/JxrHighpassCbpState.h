#ifndef JXR_HIGHPASS_CBP_STATE_H
#define JXR_HIGHPASS_CBP_STATE_H

#include "strcodec.h"

/* HP-CBP entropy and prediction dependencies owned as one explicit state. */
typedef struct JxrHighpassCbpState {
    CAdaptiveHuffman* patternHuffman;
    CAdaptiveHuffman* countHuffman;
    CCBPModel* predictionModel;
} JxrHighpassCbpState;

Void JxrHighpassCbpStateInit(JxrHighpassCbpState* state,
    CAdaptiveHuffman* patternHuffman, CAdaptiveHuffman* countHuffman,
    CCBPModel* predictionModel);
CAdaptiveHuffman* JxrHighpassCbpStateGetPatternHuffman(const JxrHighpassCbpState* state);
CAdaptiveHuffman* JxrHighpassCbpStateGetCountHuffman(const JxrHighpassCbpState* state);
CCBPModel* JxrHighpassCbpStateGetPredictionModel(const JxrHighpassCbpState* state);
Void JxrHighpassCbpStateAdapt(JxrHighpassCbpState* state);

#endif
