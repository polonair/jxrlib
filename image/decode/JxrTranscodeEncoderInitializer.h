#ifndef JXR_TRANSCODE_ENCODER_INITIALIZER_H
#define JXR_TRANSCODE_ENCODER_INITIALIZER_H

#include "windowsmediaphoto.h"
#include "strcodec.h"

typedef struct JxrTranscodeEncoderInitializationResult {
    CWMImageStrCodec* encoderCodec;
    U8* ioHeaderAllocation;
} JxrTranscodeEncoderInitializationResult;

Int JxrTranscodeEncoderInitializerInitialize(CWMImageStrCodec* decoderCodec,
    struct WMPStream* outputStream, CWMTranscodingParam* parameters,
    JxrTranscodeEncoderInitializationResult* result);

#endif
