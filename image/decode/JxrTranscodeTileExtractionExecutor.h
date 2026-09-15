#ifndef JXR_TRANSCODE_TILE_EXTRACTION_EXECUTOR_H
#define JXR_TRANSCODE_TILE_EXTRACTION_EXECUTOR_H

#include "strcodec.h"

size_t JxrTranscodeTileExtractionExecutorPacketCount(BITSTREAMFORMAT layout,
    SUBBAND subband);
Bool JxrTranscodeTileExtractionExecutorContainsTile(U32 tileColumn, U32 tileRow,
    size_t macroblockLeft, size_t macroblockRight,
    size_t macroblockTop, size_t macroblockBottom);
Int JxrTranscodeTileExtractionExecutorExecute(CWMImageStrCodec* sourceCodec,
    CWMImageStrCodec* destinationCodec, size_t macroblockLeft,
    size_t macroblockRight, size_t macroblockTop, size_t macroblockBottom);

#endif
