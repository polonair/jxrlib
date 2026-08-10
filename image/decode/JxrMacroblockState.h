#ifndef JXR_MACROBLOCK_STATE_H
#define JXR_MACROBLOCK_STATE_H

#include "strcodec.h"

/* Explicit mutable macroblock data with a temporary native commit bridge. */
typedef struct JxrMacroblockState {
    I32 dcCoefficients[MAX_CHANNELS][16];
    U8 lowpassQuantizerIndex;
    U8 highpassQuantizerIndex;
    I32 orientation;
    CWMIMBInfo* nativeMacroblock;
} JxrMacroblockState;

Void JxrMacroblockStateInit(JxrMacroblockState* state, CWMIMBInfo* nativeMacroblock);
Void JxrMacroblockStateLoadFromNative(JxrMacroblockState* state);
Void JxrMacroblockStateCommitToNative(const JxrMacroblockState* state);
Void JxrMacroblockStateClearDc(JxrMacroblockState* state, Int channelCount);
I32* JxrMacroblockStateGetDcCoefficients(JxrMacroblockState* state, Int channel);
I32 JxrMacroblockStateGetDcCoefficient(const JxrMacroblockState* state, Int channel, Int index);
Void JxrMacroblockStateSetDcCoefficient(JxrMacroblockState* state, Int channel, Int index, I32 value);
Void JxrMacroblockStateResetQuantizerIndices(JxrMacroblockState* state);
U8 JxrMacroblockStateGetLowpassQuantizerIndex(const JxrMacroblockState* state);
U8 JxrMacroblockStateGetHighpassQuantizerIndex(const JxrMacroblockState* state);
Void JxrMacroblockStateSetLowpassQuantizerIndex(JxrMacroblockState* state, U8 value);
Void JxrMacroblockStateSetHighpassQuantizerIndex(JxrMacroblockState* state, U8 value);
I32 JxrMacroblockStateGetOrientation(const JxrMacroblockState* state);

#endif
