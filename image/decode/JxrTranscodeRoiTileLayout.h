#ifndef JXR_TRANSCODE_ROI_TILE_LAYOUT_H
#define JXR_TRANSCODE_ROI_TILE_LAYOUT_H

#include "windowsmediaphoto.h"
#include "JxrTranscodeOrientationState.h"

typedef struct JxrTranscodeRoiTileLayout {
    U32 columnBoundaries[MAX_TILES];
    size_t columnCount;
    U32 rowBoundaries[MAX_TILES];
    size_t rowCount;
} JxrTranscodeRoiTileLayout;

/* Copies zero-based tile boundaries into explicit fixed-capacity layout state. */
Bool JxrTranscodeRoiTileLayoutInitialize(JxrTranscodeRoiTileLayout* layout,
    const U32* columnBoundaries, size_t columnCount,
    const U32* rowBoundaries, size_t rowCount);

/* Crops boundaries to macroblock ROI, then applies orientation reversals/transpose. */
Bool JxrTranscodeRoiTileLayoutApply(JxrTranscodeRoiTileLayout* layout,
    size_t macroblockLeft, size_t macroblockRight, size_t macroblockTop,
    size_t macroblockBottom, const JxrTranscodeOrientationState* orientation);

/* Applies orientation to extra-pixel extents after ROI geometry has been calculated. */
Void JxrTranscodeRoiTileLayoutOrientExtraPixels(size_t* left, size_t* top,
    size_t* right, size_t* bottom,
    const JxrTranscodeOrientationState* orientation);

#endif
