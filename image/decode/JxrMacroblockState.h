#ifndef JXR_MACROBLOCK_STATE_H
#define JXR_MACROBLOCK_STATE_H

#include "strcodec.h"

/* Explicit mutable data for the macroblock currently being decoded. */
typedef struct JxrMacroblockState {
    CWMIMBInfo* nativeMacroblock;
} JxrMacroblockState;

Void JxrMacroblockStateInit(JxrMacroblockState* state, CWMIMBInfo* nativeMacroblock);
Void JxrMacroblockStateClearDc(JxrMacroblockState* state, Int channelCount);
I32* JxrMacroblockStateGetDcCoefficients(JxrMacroblockState* state, Int channel);
I32 JxrMacroblockStateGetDcCoefficient(const JxrMacroblockState* state, Int channel, Int index);
Void JxrMacroblockStateSetDcCoefficient(JxrMacroblockState* state, Int channel, Int index, I32 value);
Void JxrMacroblockStateResetQuantizerIndices(JxrMacroblockState* state);
U8 JxrMacroblockStateGetLowpassQuantizerIndex(const JxrMacroblockState* state);
U8 JxrMacroblockStateGetHighpassQuantizerIndex(const JxrMacroblockState* state);
Void JxrMacroblockStateSetLowpassQuantizerIndex(JxrMacroblockState* state, U8 value);
Void JxrMacroblockStateSetHighpassQuantizerIndex(JxrMacroblockState* state, U8 value);

#endif
