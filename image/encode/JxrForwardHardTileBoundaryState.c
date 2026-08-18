#include "JxrForwardHardTileBoundaryState.h"

Void JxrForwardHardTileBoundaryStateCalculate(
    JxrForwardHardTileBoundaryState* result,
    const JxrForwardHardTileBoundaryState* previous,
    const JxrForwardHardTileBoundaryConfiguration* configuration,
    size_t macroblockColumn,
    size_t macroblockRow)
{
    *result = *previous;

    if (configuration->enabled) {
        if (macroblockColumn == 0) {
            result->isVerticalBoundary = FALSE;
            result->tileY = 0;
        }
        result->isOneMacroblockLeftOfVerticalBoundary = FALSE;
        result->isOneMacroblockRightOfVerticalBoundary = FALSE;
        if (result->tileY > 0 &&
            result->tileY <= configuration->verticalSliceCountMinusOne &&
            macroblockColumn - 1 == configuration->verticalSliceColumns[result->tileY]) {
            result->isOneMacroblockRightOfVerticalBoundary = TRUE;
        }
        if (result->tileY < configuration->verticalSliceCountMinusOne &&
            macroblockColumn == configuration->verticalSliceColumns[result->tileY + 1]) {
            result->isVerticalBoundary = TRUE;
            ++result->tileY;
        }
        else {
            result->isVerticalBoundary = FALSE;
        }
        if (result->tileY < configuration->verticalSliceCountMinusOne &&
            macroblockColumn + 1 == configuration->verticalSliceColumns[result->tileY + 1]) {
            result->isOneMacroblockLeftOfVerticalBoundary = TRUE;
        }

        if (macroblockRow == 0) {
            result->isHorizontalBoundary = FALSE;
            result->tileX = 0;
        }
        else if (result->previousMacroblockY != macroblockRow &&
            result->tileX < configuration->horizontalSliceCountMinusOne &&
            macroblockRow == configuration->horizontalSliceRows[result->tileX + 1]) {
            result->isHorizontalBoundary = TRUE;
            ++result->tileX;
        }
        else if (result->previousMacroblockY != macroblockRow) {
            result->isHorizontalBoundary = FALSE;
        }
    }
    else {
        result->isVerticalBoundary = FALSE;
        result->isHorizontalBoundary = FALSE;
        result->isOneMacroblockLeftOfVerticalBoundary = FALSE;
        result->isOneMacroblockRightOfVerticalBoundary = FALSE;
    }

    result->previousMacroblockX = macroblockColumn;
    result->previousMacroblockY = macroblockRow;
}
