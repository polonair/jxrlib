#ifndef JXR_DECODER_SESSION_RELEASER_H
#define JXR_DECODER_SESSION_RELEASER_H

#include "strcodec.h"

/* Releases all resources owned by a decoder session. */
Int JxrDecoderSessionReleaserRelease(CWMImageStrCodec* codec);

#endif
