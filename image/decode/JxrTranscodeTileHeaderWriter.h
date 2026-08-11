#ifndef JXR_TRANSCODE_TILE_HEADER_WRITER_H
#define JXR_TRANSCODE_TILE_HEADER_WRITER_H

#include "JxrTranscodeQuantizerWriter.h"

typedef struct JxrTranscodeTileHeaderState {
    Bool isSpatial;
    SUBBAND subband;
    U32 quantizerMode;
    Bool hasAlpha;
    Bool trimFlexbits;
    U8 trimFlexbitsValue;
    U8 tileId;
    size_t channelCount;
    size_t alphaChannelIndex;
    const JxrTranscodeTileQuantizerState* quantizers;
    JxrTranscodeBitSink* dcOutput;
    JxrTranscodeBitSink* lowpassOutput;
    JxrTranscodeBitSink* highpassOutput;
    JxrTranscodeBitSink* flexbitsOutput;
} JxrTranscodeTileHeaderState;

typedef struct JxrTranscodeTileHeaderResult {
    U8 lowpassQuantizerBits;
    U8 highpassQuantizerBits;
    U8 lowpassAlphaQuantizerBits;
    U8 highpassAlphaQuantizerBits;
} JxrTranscodeTileHeaderResult;

Bool JxrTranscodeTileHeaderWriterWrite(const JxrTranscodeTileHeaderState* state,
    JxrTranscodeTileHeaderResult* result);

#endif
