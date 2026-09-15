#ifndef JXR_TRANSCODE_DECODER_INITIALIZER_H
#define JXR_TRANSCODE_DECODER_INITIALIZER_H

#include "windowsmediaphoto.h"
#include "strcodec.h"
#include "decode.h"
#include "JxrTranscodeOrientationState.h"

typedef struct JxrTranscodeDecoderInitializationResult {
    ORIENTATION orientationValue;
    JxrTranscodeOrientationState orientation;
    size_t coefficientUnit;
} JxrTranscodeDecoderInitializationResult;

Int JxrTranscodeDecoderInitializerInitialize(CWMImageStrCodec* decoderCodec,
    CWMTranscodingParam* parameters, CWMDecoderParameters* decoderParameters,
    JxrTranscodeDecoderInitializationResult* result);

#endif
