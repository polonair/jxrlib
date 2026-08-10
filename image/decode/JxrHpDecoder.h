#ifndef JXR_HP_DECODER_H
#define JXR_HP_DECODER_H

#include "JxrDecoderSubbandContext.h"

Int JxrHpDecoderDecodeMacroblock(CWMImageStrCodec* codec, CCodingContext* context,
    Int macroblockX, Int macroblockY);
Int JxrHpDecoderDecodeSubband(JxrDecoderSubbandContext* state, Int macroblockX, Int macroblockY);
Bool JxrHpDecoderDecodeCbp(JxrDecoderSubbandContext* state);

#endif
