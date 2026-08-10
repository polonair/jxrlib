#include "JxrMacroblockCbpState.h"

Void JxrMacroblockCbpStateInit(JxrMacroblockCbpState* state, Int* cbpValues,
    Int* differentialValues)
{
    state->cbpValues = cbpValues;
    state->differentialValues = differentialValues;
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
