#ifndef JXR_TRANSCODE_TILE_CONTEXT_RESOLVER_H
#define JXR_TRANSCODE_TILE_CONTEXT_RESOLVER_H

#include "windowsmediaphoto.h"
#include "JxrTranscodeOrientationState.h"

typedef struct JxrTranscodeTileContextRequest {
    size_t sourceRow;
    size_t sourceColumn;
    size_t macroblockLeft;
    size_t macroblockRight;
    size_t macroblockTop;
    size_t macroblockBottom;
    size_t macroblockWidth;
    size_t macroblockHeight;
    const U32* tileColumns;
    size_t tileColumnCount;
    const U32* tileRows;
    size_t tileRowCount;
    const JxrTranscodeOrientationState* orientation;
} JxrTranscodeTileContextRequest;

typedef struct JxrTranscodeTileContextResult {
    Bool isInsideRoi;
    Int destinationRow;
    Int destinationColumn;
    Bool isTileRowStart;
    Bool isTileColumnStart;
    size_t tileRow;
    size_t tileColumn;
} JxrTranscodeTileContextResult;

Bool JxrTranscodeTileContextResolverResolve(
    const JxrTranscodeTileContextRequest* request,
    JxrTranscodeTileContextResult* result);

#endif
