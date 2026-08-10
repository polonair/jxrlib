#ifndef JXR_LOWPASS_CBP_STATE_H
#define JXR_LOWPASS_CBP_STATE_H

#include "windowsmediaphoto.h"

/* Named mutable LP-CBP state; the legacy scalar locations are implementation detail. */
typedef struct JxrLowpassCbpState {
    Int* zeroCount;
    Int* maxCount;
} JxrLowpassCbpState;

Void JxrLowpassCbpStateInit(JxrLowpassCbpState* state, Int* zeroCount, Int* maxCount);
Int JxrLowpassCbpStateGetZeroCount(const JxrLowpassCbpState* state);
Int JxrLowpassCbpStateGetMaxCount(const JxrLowpassCbpState* state);
Void JxrLowpassCbpStateObserve(JxrLowpassCbpState* state, Int cbp, Int maximumCbp);

#endif
