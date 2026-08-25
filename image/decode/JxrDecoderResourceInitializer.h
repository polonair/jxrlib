#ifndef JXR_DECODER_RESOURCE_INITIALIZER_H
#define JXR_DECODER_RESOURCE_INITIALIZER_H

#include "strcodec.h"

Int JxrDecoderResourceInitializerInitialize(CWMImageStrCodec* codec);
Int JxrDecoderResourceInitializerReleaseIo(CWMImageStrCodec* codec);
Int JxrDecoderResourceInitializerRelease(CWMImageStrCodec* codec);

#endif
