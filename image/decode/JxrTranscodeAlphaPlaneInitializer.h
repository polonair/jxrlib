#ifndef JXR_TRANSCODE_ALPHA_PLANE_INITIALIZER_H
#define JXR_TRANSCODE_ALPHA_PLANE_INITIALIZER_H

#include "windowsmediaphoto.h"
#include "strcodec.h"

typedef struct JxrTranscodeAlphaPlaneInitializationResult {
    Bool hasAlpha;
    size_t channelIndex;
} JxrTranscodeAlphaPlaneInitializationResult;

Int JxrTranscodeAlphaPlaneInitializerInitializeDecoder(
    CWMImageStrCodec* decoderCodec, CWMTranscodingParam* parameters,
    PixelI* alphaMacroblockBuffer,
    JxrTranscodeAlphaPlaneInitializationResult* result);

Int JxrTranscodeAlphaPlaneInitializerFinalizeDecoder(
    CWMImageStrCodec* decoderCodec);

Int JxrTranscodeAlphaPlaneInitializerInitializeEncoder(
    CWMImageStrCodec* decoderCodec, CWMImageStrCodec* encoderCodec,
    const CWMTranscodingParam* parameters);

#endif
