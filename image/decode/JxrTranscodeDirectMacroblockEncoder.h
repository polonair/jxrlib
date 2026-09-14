#ifndef JXR_TRANSCODE_DIRECT_MACROBLOCK_ENCODER_H
#define JXR_TRANSCODE_DIRECT_MACROBLOCK_ENCODER_H

#include "JxrTranscodeTileQuantizerState.h"

Int JxrTranscodeDirectMacroblockEncoderEncode(CWMImageStrCodec* sourceCodec,
    CWMImageStrCodec* destinationCodec, size_t macroblockLeft, size_t macroblockTop,
    Int destinationColumn, Int destinationRow,
    JxrTranscodeTileQuantizerState* quantizers, Bool hasAlpha);

#endif
