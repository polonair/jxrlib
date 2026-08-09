#ifndef JXR_HP_DECODER_H
#define JXR_HP_DECODER_H

#include "strcodec.h"

Int JxrHpDecoderDecodeMacroblock(CWMImageStrCodec* codec, CCodingContext* context,
    Int macroblockX, Int macroblockY);

#endif
