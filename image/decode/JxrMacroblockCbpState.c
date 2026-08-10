#include "JxrMacroblockCbpState.h"

Void JxrMacroblockCbpStateInit(JxrMacroblockCbpState* state, Int* cbpValues,
    Int* differentialValues)
{
    state->nativeCbpValues = cbpValues;
    state->nativeDifferentialValues = differentialValues;
    JxrMacroblockCbpStateLoadFromNative(state);
}

Void JxrMacroblockCbpStateLoadFromNative(JxrMacroblockCbpState* state)
{
    if (state->nativeCbpValues != NULL)
        memcpy(state->cbpValues, state->nativeCbpValues, sizeof(state->cbpValues));
    else
        memset(state->cbpValues, 0, sizeof(state->cbpValues));
    if (state->nativeDifferentialValues != NULL)
        memcpy(state->differentialValues, state->nativeDifferentialValues,
            sizeof(state->differentialValues));
    else
        memset(state->differentialValues, 0, sizeof(state->differentialValues));
}

Void JxrMacroblockCbpStateCommitToNative(const JxrMacroblockCbpState* state)
{
    if (state->nativeCbpValues != NULL)
        memcpy(state->nativeCbpValues, state->cbpValues, sizeof(state->cbpValues));
    if (state->nativeDifferentialValues != NULL)
        memcpy(state->nativeDifferentialValues, state->differentialValues,
            sizeof(state->differentialValues));
}

Int JxrMacroblockCbpStateGetCbp(const JxrMacroblockCbpState* state, Int plane)
{
    return state->cbpValues[plane];
}

Void JxrMacroblockCbpStateSetCbp(JxrMacroblockCbpState* state, Int plane, Int value)
{
    state->cbpValues[plane] = value;
}

Int JxrMacroblockCbpStateGetDifferential(const JxrMacroblockCbpState* state, Int plane)
{
    return state->differentialValues[plane];
}

Void JxrMacroblockCbpStateSetDifferential(JxrMacroblockCbpState* state, Int plane, Int value)
{
    state->differentialValues[plane] = value;
}
