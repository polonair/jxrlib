#ifndef JXR_DECODER_UV_INTERPOLATOR_H
#define JXR_DECODER_UV_INTERPOLATOR_H

#include "strcodec.h"

/* Expands the current U/V macroblock from 4:2:2 or 4:2:0 resolution. */
Void JxrDecoderUvInterpolatorInterpolate(CWMImageStrCodec* codec);

#endif
