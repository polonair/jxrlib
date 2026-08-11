#include "JxrTranscodeTileExtractionDecision.h"

Bool JxrTranscodeTileExtractionDecisionIsBoundary(const U32* tilePositions,
    U32 tileCount, U32 macroblockCount, U32 pixelPosition)
{
    U32 tile;

    if (tilePositions == NULL) return FALSE;
    for (tile = 0; tile < tileCount; ++tile)
        if (pixelPosition == tilePositions[tile] * 16) return TRUE;
    return (pixelPosition + 15) / 16 >= macroblockCount ? TRUE : FALSE;
}

Bool JxrTranscodeTileExtractionDecisionCanUseFastPath(
    JxrTranscodeTileExtractionDecision* decision)
{
    size_t right;
    size_t bottom;

    if (decision == NULL) return FALSE;
    if (!decision->ignoreOverlap && decision->sourceOverlap == OL_NONE)
        decision->ignoreOverlap = TRUE;
    if (!decision->ignoreOverlap || decision->hasTransform ||
        decision->sourceLayout != decision->targetLayout) return FALSE;
    if (decision->targetLayout == SPATIAL && decision->targetSubband != decision->sourceSubband)
        return FALSE;
    right = decision->roiLeftPixels + decision->roiWidthPixels + decision->extraLeftPixels;
    bottom = decision->roiTopPixels + decision->roiHeightPixels + decision->extraTopPixels;
    return JxrTranscodeTileExtractionDecisionIsBoundary(decision->tileColumns,
        decision->tileColumnCount, decision->macroblockWidth,
        (U32)(decision->roiLeftPixels + decision->extraLeftPixels)) &&
        JxrTranscodeTileExtractionDecisionIsBoundary(decision->tileRows,
            decision->tileRowCount, decision->macroblockHeight,
            (U32)(decision->roiTopPixels + decision->extraTopPixels)) &&
        JxrTranscodeTileExtractionDecisionIsBoundary(decision->tileColumns,
            decision->tileColumnCount, decision->macroblockWidth, (U32)right) &&
        JxrTranscodeTileExtractionDecisionIsBoundary(decision->tileRows,
            decision->tileRowCount, decision->macroblockHeight, (U32)bottom);
}
