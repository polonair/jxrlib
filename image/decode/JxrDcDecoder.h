#ifndef JXR_DC_DECODER_H
#define JXR_DC_DECODER_H

#include "JxrDecoderSubbandContext.h"

/* Decoder-facing implementation; DecodeMacroblockDC remains the compatibility entry point. */
Int JxrDcDecoderDecodeMacroblock(CWMImageStrCodec* codec, CCodingContext* context,
    Int macroblockX, Int macroblockY);
Int JxrDcDecoderDecodeSubband(JxrDecoderSubbandContext* state, Int macroblockX, Int macroblockY);

#endif
