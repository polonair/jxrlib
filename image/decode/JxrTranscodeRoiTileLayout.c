#include "JxrTranscodeRoiTileLayout.h"

static Void JxrTranscodeRoiTileLayoutSwapSize(size_t* left, size_t* right)
{
    size_t temporary = *left;
    *left = *right;
    *right = temporary;
}

static Bool JxrTranscodeRoiTileLayoutSelect(U32* boundaries, size_t* count,
    size_t rangeStart, size_t rangeEnd)
{
    U32 selected[MAX_TILES];
    size_t sourceIndex;
    size_t selectedCount = 0;

    if (*count == 0 || *count > MAX_TILES || rangeStart >= rangeEnd)
        return FALSE;

    for (sourceIndex = 0; sourceIndex < *count; sourceIndex++) {
        size_t boundary = boundaries[sourceIndex];
        if (boundary >= rangeStart && boundary < rangeEnd) {
            if (selectedCount == MAX_TILES)
                return FALSE;
            selected[selectedCount++] = (U32)(boundary - rangeStart);
        }
    }

    if (selectedCount == 0) {
        boundaries[0] = 0;
        *count = 1;
        return TRUE;
    }

    if (selected[0] == 0) {
        for (sourceIndex = 0; sourceIndex < selectedCount; sourceIndex++)
            boundaries[sourceIndex] = selected[sourceIndex];
        *count = selectedCount;
        return TRUE;
    }

    if (selectedCount == MAX_TILES)
        return FALSE;
    boundaries[0] = 0;
    for (sourceIndex = 0; sourceIndex < selectedCount; sourceIndex++)
        boundaries[sourceIndex + 1] = selected[sourceIndex];
    *count = selectedCount + 1;
    return TRUE;
}

static Void JxrTranscodeRoiTileLayoutReverse(U32* boundaries, size_t count,
    size_t macroblockLength)
{
    U32 reversed[MAX_TILES];
    size_t boundaryIndex;

    for (boundaryIndex = 0; boundaryIndex < count; boundaryIndex++)
        reversed[boundaryIndex] = (U32)(macroblockLength - boundaries[boundaryIndex]);
    boundaries[0] = 0;
    for (boundaryIndex = 1; boundaryIndex < count; boundaryIndex++)
        boundaries[boundaryIndex] = reversed[count - boundaryIndex];
}

Bool JxrTranscodeRoiTileLayoutInitialize(JxrTranscodeRoiTileLayout* layout,
    const U32* columnBoundaries, size_t columnCount,
    const U32* rowBoundaries, size_t rowCount)
{
    size_t boundaryIndex;

    if (layout == NULL || columnBoundaries == NULL || rowBoundaries == NULL ||
        columnCount == 0 || rowCount == 0 || columnCount > MAX_TILES ||
        rowCount > MAX_TILES)
        return FALSE;

    for (boundaryIndex = 0; boundaryIndex < columnCount; boundaryIndex++)
        layout->columnBoundaries[boundaryIndex] = columnBoundaries[boundaryIndex];
    for (boundaryIndex = 0; boundaryIndex < rowCount; boundaryIndex++)
        layout->rowBoundaries[boundaryIndex] = rowBoundaries[boundaryIndex];
    layout->columnCount = columnCount;
    layout->rowCount = rowCount;
    return TRUE;
}

Bool JxrTranscodeRoiTileLayoutApply(JxrTranscodeRoiTileLayout* layout,
    size_t macroblockLeft, size_t macroblockRight, size_t macroblockTop,
    size_t macroblockBottom, const JxrTranscodeOrientationState* orientation)
{
    U32 columns[MAX_TILES];
    size_t columnIndex;
    size_t originalColumnCount;

    if (layout == NULL || orientation == NULL || macroblockLeft >= macroblockRight ||
        macroblockTop >= macroblockBottom ||
        !JxrTranscodeRoiTileLayoutSelect(layout->columnBoundaries,
            &layout->columnCount, macroblockLeft, macroblockRight) ||
        !JxrTranscodeRoiTileLayoutSelect(layout->rowBoundaries,
            &layout->rowCount, macroblockTop, macroblockBottom))
        return FALSE;

    if (orientation->flipHorizontal)
        JxrTranscodeRoiTileLayoutReverse(layout->columnBoundaries,
            layout->columnCount, macroblockRight - macroblockLeft);
    if (orientation->flipVertical)
        JxrTranscodeRoiTileLayoutReverse(layout->rowBoundaries,
            layout->rowCount, macroblockBottom - macroblockTop);
    if (!orientation->transpose)
        return TRUE;

    originalColumnCount = layout->columnCount;
    for (columnIndex = 0; columnIndex < originalColumnCount; columnIndex++)
        columns[columnIndex] = layout->columnBoundaries[columnIndex];
    for (columnIndex = 0; columnIndex < layout->rowCount; columnIndex++)
        layout->columnBoundaries[columnIndex] = layout->rowBoundaries[columnIndex];
    for (columnIndex = 0; columnIndex < originalColumnCount; columnIndex++)
        layout->rowBoundaries[columnIndex] = columns[columnIndex];
    JxrTranscodeRoiTileLayoutSwapSize(&layout->columnCount, &layout->rowCount);
    return TRUE;
}

Void JxrTranscodeRoiTileLayoutOrientExtraPixels(size_t* left, size_t* top,
    size_t* right, size_t* bottom,
    const JxrTranscodeOrientationState* orientation)
{
    if (orientation->flipHorizontal)
        JxrTranscodeRoiTileLayoutSwapSize(left, right);
    if (orientation->flipVertical)
        JxrTranscodeRoiTileLayoutSwapSize(top, bottom);
    if (orientation->transpose) {
        JxrTranscodeRoiTileLayoutSwapSize(left, top);
        JxrTranscodeRoiTileLayoutSwapSize(right, bottom);
    }
}
