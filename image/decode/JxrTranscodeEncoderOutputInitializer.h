#ifndef JXR_TRANSCODE_ENCODER_OUTPUT_INITIALIZER_H
#define JXR_TRANSCODE_ENCODER_OUTPUT_INITIALIZER_H

#include "windowsmediaphoto.h"
#include "strcodec.h"
#include "JxrTranscodeOrientationState.h"
#include "JxrTranscodeTileQuantizerState.h"

typedef struct JxrTranscodeEncoderOutputInitializationResult {
    JxrTranscodeTileQuantizerState* tileQuantizers;
    size_t tileQuantizerCount;
    Bool usedFastTileExtraction;
} JxrTranscodeEncoderOutputInitializationResult;

Int JxrTranscodeEncoderOutputInitializerInitialize(CWMImageStrCodec* decoderCodec,
    CWMImageStrCodec* encoderCodec, CWMTranscodingParam* parameters,
    ORIENTATION orientationValue, const JxrTranscodeOrientationState* orientation,
    JxrTranscodeEncoderOutputInitializationResult* result);

Void JxrTranscodeEncoderOutputInitializerRelease(
    JxrTranscodeEncoderOutputInitializationResult* result);

#endif
