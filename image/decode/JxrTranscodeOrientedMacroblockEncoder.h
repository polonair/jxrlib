#ifndef JXR_TRANSCODE_ORIENTED_MACROBLOCK_ENCODER_H
#define JXR_TRANSCODE_ORIENTED_MACROBLOCK_ENCODER_H

#include "JxrTranscodeMacroblockTransform.h"
#include "JxrTranscodeTileHeaderEmitter.h"

typedef struct JxrTranscodeOrientedMacroblockEncoderRequest {
    CWMImageStrCodec* destinationCodec;
    const CWMImageStrCodec* sourceAlphaCodec;
    CWMImageStrCodec* destinationAlphaCodec;
    CWMIMBInfo* primaryMacroblocks;
    PixelI* primaryCoefficients;
    size_t coefficientUnit;
    size_t macroblockCount;
    PixelI* destinationCoefficients;
    const JxrTranscodeOrientationState* orientation;
    JxrTranscodeTileQuantizerState* tileQuantizers;
    size_t tileQuantizerCount;
    size_t tileColumnCount;
    Bool hasAlpha;
    CWMIMBInfo* alphaMacroblocks;
    PixelI* alphaCoefficients;
    PixelI* alphaDestinationCoefficients;
} JxrTranscodeOrientedMacroblockEncoderRequest;

Int JxrTranscodeOrientedMacroblockEncoderEncode(
    const JxrTranscodeOrientedMacroblockEncoderRequest* request);

#endif
