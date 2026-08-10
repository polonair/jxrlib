#ifndef JXR_MACROBLOCK_CBP_STATE_H
#define JXR_MACROBLOCK_CBP_STATE_H

#include "windowsmediaphoto.h"

/* Explicit per-plane CBP state used by HP decoding and CBP prediction. */
typedef struct JxrMacroblockCbpState {
    Int* cbpValues;
    Int* differentialValues;
} JxrMacroblockCbpState;

Void JxrMacroblockCbpStateInit(JxrMacroblockCbpState* state, Int* cbpValues,
    Int* differentialValues);
Int JxrMacroblockCbpStateGetCbp(const JxrMacroblockCbpState* state, Int plane);
Void JxrMacroblockCbpStateSetCbp(JxrMacroblockCbpState* state, Int plane, Int value);
Int JxrMacroblockCbpStateGetDifferential(const JxrMacroblockCbpState* state, Int plane);
Void JxrMacroblockCbpStateSetDifferential(JxrMacroblockCbpState* state, Int plane, Int value);

#endif
