#ifndef JXR_TRANSCODE_TILE_EXTRACTION_DECISION_H
#define JXR_TRANSCODE_TILE_EXTRACTION_DECISION_H

#include "strcodec.h"

typedef struct JxrTranscodeTileExtractionDecision {
    const U32* tileColumns;
    U32 tileColumnCount;
    U32 macroblockWidth;
    const U32* tileRows;
    U32 tileRowCount;
    U32 macroblockHeight;
    size_t roiLeftPixels;
    size_t roiTopPixels;
    size_t roiWidthPixels;
    size_t roiHeightPixels;
    size_t extraLeftPixels;
    size_t extraTopPixels;
    OVERLAP sourceOverlap;
    Bool ignoreOverlap;
    Bool hasTransform;
    BITSTREAMFORMAT sourceLayout;
    BITSTREAMFORMAT targetLayout;
    SUBBAND sourceSubband;
    SUBBAND targetSubband;
} JxrTranscodeTileExtractionDecision;

Bool JxrTranscodeTileExtractionDecisionIsBoundary(const U32* tilePositions,
    U32 tileCount, U32 macroblockCount, U32 pixelPosition);
Bool JxrTranscodeTileExtractionDecisionCanUseFastPath(
    JxrTranscodeTileExtractionDecision* decision);

#endif
