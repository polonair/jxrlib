#ifndef JXR_TRANSCODE_ROI_INITIALIZER_H
#define JXR_TRANSCODE_ROI_INITIALIZER_H

#include "windowsmediaphoto.h"
#include "strcodec.h"
#include "JxrTranscodeOrientationState.h"

typedef struct JxrTranscodeRoiInitializationResult {
    size_t macroblockLeft;
    size_t macroblockRight;
    size_t macroblockTop;
    size_t macroblockBottom;
    size_t macroblockWidth;
    size_t macroblockHeight;
} JxrTranscodeRoiInitializationResult;

Int JxrTranscodeRoiInitializerInitialize(CWMImageStrCodec* decoderCodec,
    CWMImageStrCodec* encoderCodec, CWMTranscodingParam* parameters,
    const JxrTranscodeOrientationState* orientation,
    JxrTranscodeRoiInitializationResult* result);

#endif
