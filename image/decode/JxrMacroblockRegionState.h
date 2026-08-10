#ifndef JXR_MACROBLOCK_REGION_STATE_H
#define JXR_MACROBLOCK_REGION_STATE_H

#include "strcodec.h"

#define JXR_MACROBLOCK_EDGE_PIXELS 16
#define JXR_ROI_TRANSFORM_MARGIN_PIXELS 25

/* Immutable coordinates used by the macroblock ROI and transform decisions. */
typedef struct JxrMacroblockRegionState {
    size_t macroblockX;
    size_t macroblockY;
    size_t tileLeftMacroblock;
    size_t tileTopMacroblock;
    size_t tileRightMacroblock;
    size_t tileBottomMacroblock;
    size_t roiLeftPixels;
    size_t roiTopPixels;
    size_t roiRightPixels;
    size_t roiBottomPixels;
} JxrMacroblockRegionState;

Bool JxrMacroblockRegionStateIsTileStart(const JxrMacroblockRegionState* state);
Bool JxrMacroblockRegionStateIntersectsEntropyRoi(const JxrMacroblockRegionState* state,
    OVERLAP overlap);
Bool JxrMacroblockRegionStateShouldTransform(const JxrMacroblockRegionState* state,
    Bool decodeFullFrame);

#endif
