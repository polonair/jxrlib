#include "JxrMacroblockRegionState.h"

static size_t JxrMacroblockRegionStateOverlapExtent(OVERLAP overlap)
{
    return overlap == OL_NONE ? 0 : overlap == OL_ONE ? 2 : 10;
}

Bool JxrMacroblockRegionStateIsTileStart(const JxrMacroblockRegionState* state)
{
    return state->macroblockX == state->tileLeftMacroblock;
}

Bool JxrMacroblockRegionStateIntersectsEntropyRoi(const JxrMacroblockRegionState* state,
    OVERLAP overlap)
{
    size_t overlapExtent = JxrMacroblockRegionStateOverlapExtent(overlap);
    size_t tileLeft = state->tileLeftMacroblock * JXR_MACROBLOCK_EDGE_PIXELS;
    size_t tileTop = state->tileTopMacroblock * JXR_MACROBLOCK_EDGE_PIXELS;
    size_t tileRight = state->tileRightMacroblock * JXR_MACROBLOCK_EDGE_PIXELS;
    size_t tileBottom = state->tileBottomMacroblock * JXR_MACROBLOCK_EDGE_PIXELS;
    size_t macroblockTop = state->macroblockY * JXR_MACROBLOCK_EDGE_PIXELS;

    return !(state->roiLeftPixels >= tileRight + overlapExtent ||
        state->roiTopPixels >= tileBottom + overlapExtent ||
        tileLeft > state->roiRightPixels + overlapExtent ||
        tileTop > state->roiBottomPixels + overlapExtent ||
        macroblockTop > state->roiBottomPixels + overlapExtent);
}

Bool JxrMacroblockRegionStateShouldTransform(const JxrMacroblockRegionState* state,
    Bool decodeFullFrame)
{
    size_t macroblockLeft = state->macroblockX * JXR_MACROBLOCK_EDGE_PIXELS;
    size_t macroblockTop = state->macroblockY * JXR_MACROBLOCK_EDGE_PIXELS;

    if (decodeFullFrame) return TRUE;
    return !(macroblockLeft > state->roiRightPixels + JXR_ROI_TRANSFORM_MARGIN_PIXELS ||
        macroblockLeft + JXR_ROI_TRANSFORM_MARGIN_PIXELS < state->roiLeftPixels ||
        macroblockTop > state->roiBottomPixels + JXR_ROI_TRANSFORM_MARGIN_PIXELS ||
        macroblockTop + JXR_ROI_TRANSFORM_MARGIN_PIXELS < state->roiTopPixels);
}
