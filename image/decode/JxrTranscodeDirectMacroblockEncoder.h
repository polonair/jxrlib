#ifndef JXR_TRANSCODE_DIRECT_MACROBLOCK_ENCODER_H
#define JXR_TRANSCODE_DIRECT_MACROBLOCK_ENCODER_H

#include "JxrTranscodePlanePair.h"
#include "JxrTranscodeTileQuantizerState.h"

Int JxrTranscodeDirectMacroblockEncoderEncode(
    const JxrTranscodePlanePair* sourcePlanes,
    const JxrTranscodePlanePair* destinationPlanes, size_t macroblockLeft, size_t macroblockTop,
    Int destinationColumn, Int destinationRow,
    JxrTranscodeTileQuantizerState* quantizers);

#endif
