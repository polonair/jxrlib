#ifndef JXR_INVERSE_TRANSFORM_DECODER_H
#define JXR_INVERSE_TRANSFORM_DECODER_H

#include "strcodec.h"

/* Explicit decoder orchestration for one inverse-transform macroblock. */
Int JxrInverseTransformDecoderProcessNormalMacroblock(CWMImageStrCodec* codec);
Int JxrInverseTransformDecoderProcessAlternateMacroblock(CWMImageStrCodec* codec);

#endif
