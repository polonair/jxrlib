#ifndef JXR_ENCODER_MACROBLOCK_PROCESSOR_H
#define JXR_ENCODER_MACROBLOCK_PROCESSOR_H

#include "strcodec.h"

/* Immutable processing decisions for the current input macroblock. */
typedef struct JxrEncoderMacroblockProcessState {
    Bool encodesPreviousMacroblock;
    Bool processesSecondaryCodec;
    Int previousMacroblockX;
    Int previousMacroblockY;
} JxrEncoderMacroblockProcessState;

Void JxrEncoderMacroblockProcessStateInitialize(
    JxrEncoderMacroblockProcessState* state,
    const CWMImageStrCodec* primaryCodec);

Int JxrEncoderMacroblockProcessorProcess(CWMImageStrCodec* primaryCodec);

#endif
