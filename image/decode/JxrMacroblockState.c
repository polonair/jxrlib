#include "JxrMacroblockState.h"

Void JxrMacroblockStateInit(JxrMacroblockState* state, CWMIMBInfo* nativeMacroblock)
{
    state->nativeMacroblock = nativeMacroblock;
}

Void JxrMacroblockStateClearDc(JxrMacroblockState* state, Int channelCount)
{
    Int channel;
    for (channel = 0; channel < channelCount; ++channel)
        memset(state->nativeMacroblock->iBlockDC[channel], 0, 16 * sizeof(I32));
}

I32* JxrMacroblockStateGetDcCoefficients(JxrMacroblockState* state, Int channel)
{
    return state->nativeMacroblock->iBlockDC[channel];
}

Void JxrMacroblockStateResetQuantizerIndices(JxrMacroblockState* state)
{
    state->nativeMacroblock->iQIndexLP = 0;
    state->nativeMacroblock->iQIndexHP = 0;
}

U8 JxrMacroblockStateGetLowpassQuantizerIndex(const JxrMacroblockState* state)
{
    return state->nativeMacroblock->iQIndexLP;
}

U8 JxrMacroblockStateGetHighpassQuantizerIndex(const JxrMacroblockState* state)
{
    return state->nativeMacroblock->iQIndexHP;
}

Void JxrMacroblockStateSetLowpassQuantizerIndex(JxrMacroblockState* state, U8 value)
{
    state->nativeMacroblock->iQIndexLP = value;
}

Void JxrMacroblockStateSetHighpassQuantizerIndex(JxrMacroblockState* state, U8 value)
{
    state->nativeMacroblock->iQIndexHP = value;
}
