#ifndef JXR_LP_DECODER_H
#define JXR_LP_DECODER_H

#include "JxrDecoderSubbandContext.h"

Int JxrLpDecoderDecodeMacroblock(CWMImageStrCodec* codec, CCodingContext* context,
    Int macroblockX, Int macroblockY);
Int JxrLpDecoderDecodeSubband(JxrDecoderSubbandContext* state, Int macroblockX, Int macroblockY);

#endif
