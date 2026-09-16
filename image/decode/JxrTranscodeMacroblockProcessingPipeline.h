#ifndef JXR_TRANSCODE_MACROBLOCK_PROCESSING_PIPELINE_H
#define JXR_TRANSCODE_MACROBLOCK_PROCESSING_PIPELINE_H

#include "windowsmediaphoto.h"
#include "strcodec.h"
#include "JxrTranscodeOrientationState.h"
#include "JxrTranscodeTileQuantizerState.h"

typedef struct JxrTranscodeMacroblockProcessingPipeline {
    CWMImageStrCodec* decoderCodec;
    CWMImageStrCodec* encoderCodec;
    CWMTranscodingParam* parameters;
    PixelI* macroblockBuffer;
    PixelI* alphaMacroblockBuffer;
    size_t coefficientUnit;
    size_t alphaChannelIndex;
    size_t macroblockLeft;
    size_t macroblockRight;
    size_t macroblockTop;
    size_t macroblockBottom;
    size_t macroblockWidth;
    size_t macroblockHeight;
    ORIENTATION orientationValue;
    const JxrTranscodeOrientationState* orientation;
    JxrTranscodeTileQuantizerState* tileQuantizers;
    size_t tileQuantizerCount;
    PixelI* primaryFrameBuffer;
    PixelI* alphaFrameBuffer;
    CWMIMBInfo* primaryFrameMacroblocks;
    CWMIMBInfo* alphaFrameMacroblocks;
    Bool usedFastTileExtraction;
} JxrTranscodeMacroblockProcessingPipeline;

Int JxrTranscodeMacroblockProcessingPipelineExecute(
    JxrTranscodeMacroblockProcessingPipeline* pipeline);

#endif
