#ifndef JXR_TRANSCODE_SESSION_FACTORY_H
#define JXR_TRANSCODE_SESSION_FACTORY_H

#include "strcodec.h"

Int JxrTranscodeSessionFactoryCreateCodec(struct WMPStream* stream,
    CWMImageStrCodec** codec);

#endif
