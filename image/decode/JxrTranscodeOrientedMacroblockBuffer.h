#ifndef JXR_TRANSCODE_ORIENTED_MACROBLOCK_BUFFER_H
#define JXR_TRANSCODE_ORIENTED_MACROBLOCK_BUFFER_H

#include "strcodec.h"
#include "JxrTranscodeOrientationState.h"

typedef struct JxrTranscodeOrientedMacroblockBufferRequest {
    const CWMIMBInfo* primaryMacroblock;
    const PixelI* primaryCoefficients;
    size_t primaryCoefficientCount;
    CWMIMBInfo* primaryFrameMacroblocks;
    size_t primaryFrameMacroblockCount;
    PixelI* primaryFrameCoefficients;
    size_t primaryFrameCoefficientCount;
    Int destinationRow;
    Int destinationColumn;
    size_t sourceMacroblockWidth;
    size_t sourceMacroblockHeight;
    const JxrTranscodeOrientationState* orientation;
    Bool hasAlpha;
    const CWMIMBInfo* alphaMacroblock;
    const PixelI* alphaCoefficients;
    size_t alphaCoefficientCount;
    CWMIMBInfo* alphaFrameMacroblocks;
    size_t alphaFrameMacroblockCount;
    PixelI* alphaFrameCoefficients;
    size_t alphaFrameCoefficientCount;
} JxrTranscodeOrientedMacroblockBufferRequest;

Bool JxrTranscodeOrientedMacroblockBufferStore(
    const JxrTranscodeOrientedMacroblockBufferRequest* request);

#endif
