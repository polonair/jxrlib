#include "JxrMacroblockState.h"

Void JxrMacroblockStateInit(JxrMacroblockState* state, CWMIMBInfo* nativeMacroblock)
{
    state->nativeMacroblock = nativeMacroblock;
    JxrMacroblockStateLoadFromNative(state);
}

Void JxrMacroblockStateLoadFromNative(JxrMacroblockState* state)
{
    if (state->nativeMacroblock == NULL) {
        memset(state->dcCoefficients, 0, sizeof(state->dcCoefficients));
        state->lowpassQuantizerIndex = 0;
        state->highpassQuantizerIndex = 0;
        state->orientation = 0;
        return;
    }
    memcpy(state->dcCoefficients, state->nativeMacroblock->iBlockDC,
        sizeof(state->dcCoefficients));
    state->lowpassQuantizerIndex = state->nativeMacroblock->iQIndexLP;
    state->highpassQuantizerIndex = state->nativeMacroblock->iQIndexHP;
    state->orientation = state->nativeMacroblock->iOrientation;
}

Void JxrMacroblockStateCommitToNative(const JxrMacroblockState* state)
{
    if (state->nativeMacroblock == NULL) return;
    memcpy(state->nativeMacroblock->iBlockDC, state->dcCoefficients,
        sizeof(state->dcCoefficients));
    state->nativeMacroblock->iQIndexLP = state->lowpassQuantizerIndex;
    state->nativeMacroblock->iQIndexHP = state->highpassQuantizerIndex;
}

Void JxrMacroblockStateClearDc(JxrMacroblockState* state, Int channelCount)
{
    Int channel;
    for (channel = 0; channel < channelCount; ++channel)
        memset(state->dcCoefficients[channel], 0, 16 * sizeof(I32));
}

I32* JxrMacroblockStateGetDcCoefficients(JxrMacroblockState* state, Int channel)
{
    return state->dcCoefficients[channel];
}

I32 JxrMacroblockStateGetDcCoefficient(const JxrMacroblockState* state, Int channel, Int index)
{
    return state->dcCoefficients[channel][index];
}

Void JxrMacroblockStateSetDcCoefficient(JxrMacroblockState* state, Int channel, Int index, I32 value)
{
    state->dcCoefficients[channel][index] = value;
}

Void JxrMacroblockStateResetQuantizerIndices(JxrMacroblockState* state)
{
    state->lowpassQuantizerIndex = 0;
    state->highpassQuantizerIndex = 0;
}

U8 JxrMacroblockStateGetLowpassQuantizerIndex(const JxrMacroblockState* state)
{
    return state->lowpassQuantizerIndex;
}

U8 JxrMacroblockStateGetHighpassQuantizerIndex(const JxrMacroblockState* state)
{
    return state->highpassQuantizerIndex;
}

Void JxrMacroblockStateSetLowpassQuantizerIndex(JxrMacroblockState* state, U8 value)
{
    state->lowpassQuantizerIndex = value;
}

Void JxrMacroblockStateSetHighpassQuantizerIndex(JxrMacroblockState* state, U8 value)
{
    state->highpassQuantizerIndex = value;
}

I32 JxrMacroblockStateGetOrientation(const JxrMacroblockState* state)
{
    return state->orientation;
}
