#ifndef JXR_DECODER_CODEC_STATE_INITIALIZER_H
#define JXR_DECODER_CODEC_STATE_INITIALIZER_H

#include "strcodec.h"

/* Initializes state shared by the primary and secondary decoder planes. */
Void JxrDecoderCodecStateInitializerInitialize(CWMImageStrCodec* codec,
    const CCoreParameters* parameters, const CWMImageStrCodec* templateCodec);

#endif
