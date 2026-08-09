#ifndef JXR_LP_DECODER_H
#define JXR_LP_DECODER_H

#include "strcodec.h"

Int JxrLpDecoderDecodeMacroblock(CWMImageStrCodec* codec, CCodingContext* context,
    Int macroblockX, Int macroblockY);

#endif
