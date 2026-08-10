#ifndef JXR_MACROBLOCK_CBP_STATE_H
#define JXR_MACROBLOCK_CBP_STATE_H

#include "windowsmediaphoto.h"

/* Explicit per-plane CBP state with a temporary native commit bridge. */
typedef struct JxrMacroblockCbpState {
    Int cbpValues[MAX_CHANNELS];
    Int differentialValues[MAX_CHANNELS];
    Int* nativeCbpValues;
    Int* nativeDifferentialValues;
} JxrMacroblockCbpState;

Void JxrMacroblockCbpStateInit(JxrMacroblockCbpState* state, Int* cbpValues,
    Int* differentialValues);
Void JxrMacroblockCbpStateLoadFromNative(JxrMacroblockCbpState* state);
Void JxrMacroblockCbpStateCommitToNative(const JxrMacroblockCbpState* state);
Int JxrMacroblockCbpStateGetCbp(const JxrMacroblockCbpState* state, Int plane);
Void JxrMacroblockCbpStateSetCbp(JxrMacroblockCbpState* state, Int plane, Int value);
Int JxrMacroblockCbpStateGetDifferential(const JxrMacroblockCbpState* state, Int plane);
Void JxrMacroblockCbpStateSetDifferential(JxrMacroblockCbpState* state, Int plane, Int value);

#endif
