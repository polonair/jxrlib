#ifndef JXR_TRANSCODE_TILE_EXTRACTION_EXECUTOR_H
#define JXR_TRANSCODE_TILE_EXTRACTION_EXECUTOR_H

#include "JxrTranscodePlanePair.h"

typedef struct JxrTranscodeTileExtractionRequest {
    JxrTranscodePlanePair sourcePlanes;
    JxrTranscodePlanePair destinationPlanes;
    size_t macroblockLeft;
    size_t macroblockRight;
    size_t macroblockTop;
    size_t macroblockBottom;
} JxrTranscodeTileExtractionRequest;

size_t JxrTranscodeTileExtractionExecutorPacketCount(BITSTREAMFORMAT layout,
    SUBBAND subband);
Bool JxrTranscodeTileExtractionExecutorContainsTile(U32 tileColumn, U32 tileRow,
    size_t macroblockLeft, size_t macroblockRight,
    size_t macroblockTop, size_t macroblockBottom);
Int JxrTranscodeTileExtractionExecutorExecute(
    const JxrTranscodeTileExtractionRequest* request);

#endif
