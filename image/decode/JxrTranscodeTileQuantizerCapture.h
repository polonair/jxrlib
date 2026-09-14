#ifndef JXR_TRANSCODE_TILE_QUANTIZER_CAPTURE_H
#define JXR_TRANSCODE_TILE_QUANTIZER_CAPTURE_H

#include "JxrTranscodeTileQuantizerState.h"

typedef struct JxrTranscodeTileQuantizerCaptureRequest {
    JxrTranscodeTileQuantizerState* states;
    size_t stateCount;
    size_t destinationTileRow;
    size_t destinationTileColumn;
    size_t destinationTileColumnCount;
    Bool storeByDestinationTile;
    const CWMITile* primaryTile;
    const CWMITile* alphaTile;
    size_t primaryChannelCount;
    size_t alphaChannelIndex;
    SUBBAND subband;
    Bool hasAlpha;
} JxrTranscodeTileQuantizerCaptureRequest;

Bool JxrTranscodeTileQuantizerCaptureSelectIndex(size_t stateCount,
    size_t tileRow, size_t tileColumn, size_t tileColumnCount,
    Bool storeByDestinationTile, size_t* stateIndex);
Bool JxrTranscodeTileQuantizerCaptureCapture(
    const JxrTranscodeTileQuantizerCaptureRequest* request);

#endif
