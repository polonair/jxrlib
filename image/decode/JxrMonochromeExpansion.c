#include "JxrMonochromeExpansion.h"

Void JxrMonochromeExpansionReplicateByte(U8* buffer, size_t strideBytes,
    size_t width, size_t height, size_t componentsPerPixel)
{
    size_t row;
    size_t column;
    for (row = 0; row < height; ++row) {
        U8* pixel = (U8*)((U8*)buffer + strideBytes * row);
        for (column = 0; column < width; ++column) {
            pixel[2] = pixel[1] = pixel[0];
            pixel += componentsPerPixel;
        }
    }
}

Void JxrMonochromeExpansionReplicateUInt16(U16* buffer, size_t strideBytes,
    size_t width, size_t height, size_t componentsPerPixel)
{
    size_t row;
    size_t column;
    for (row = 0; row < height; ++row) {
        U16* pixel = (U16*)((U8*)buffer + strideBytes * row);
        for (column = 0; column < width; ++column) {
            pixel[2] = pixel[1] = pixel[0];
            pixel += componentsPerPixel;
        }
    }
}

Void JxrMonochromeExpansionReplicateUInt32(U32* buffer, size_t strideBytes,
    size_t width, size_t height, size_t componentsPerPixel)
{
    size_t row;
    size_t column;
    for (row = 0; row < height; ++row) {
        U32* pixel = (U32*)((U8*)buffer + strideBytes * row);
        for (column = 0; column < width; ++column) {
            pixel[2] = pixel[1] = pixel[0];
            pixel += componentsPerPixel;
        }
    }
}

Void JxrMonochromeExpansionReplicateByteAtOffsets(U8* buffer, const size_t* xOffsets,
    const size_t* yOffsets, size_t firstRow, size_t endRow, size_t firstColumn,
    size_t endColumn)
{
    size_t row;
    size_t column;
    for (row = firstRow; row < endRow; ++row)
        for (column = firstColumn; column < endColumn; ++column) {
            U8* pixel = buffer + yOffsets[row] + xOffsets[column];
            pixel[2] = pixel[1] = pixel[0];
        }
}

Void JxrMonochromeExpansionReplicateUInt16AtOffsets(U16* buffer, const size_t* xOffsets,
    const size_t* yOffsets, size_t firstRow, size_t endRow, size_t firstColumn,
    size_t endColumn)
{
    size_t row;
    size_t column;
    for (row = firstRow; row < endRow; ++row)
        for (column = firstColumn; column < endColumn; ++column) {
            U16* pixel = buffer + yOffsets[row] + xOffsets[column];
            pixel[2] = pixel[1] = pixel[0];
        }
}

Void JxrMonochromeExpansionReplicateUInt32AtOffsets(U32* buffer, const size_t* xOffsets,
    const size_t* yOffsets, size_t firstRow, size_t endRow, size_t firstColumn,
    size_t endColumn)
{
    size_t row;
    size_t column;
    for (row = firstRow; row < endRow; ++row)
        for (column = firstColumn; column < endColumn; ++column) {
            U32* pixel = buffer + yOffsets[row] + xOffsets[column];
            pixel[2] = pixel[1] = pixel[0];
        }
}

Void JxrMonochromeExpansionReplicateByteAtScaledOffsets(U8* buffer, const size_t* xOffsets,
    const size_t* yOffsets, size_t firstRow, size_t endRow, size_t firstColumn,
    size_t endColumn, size_t scale, size_t scaleBits, size_t sourceIndex, size_t blueIndex)
{
    size_t row;
    size_t column;
    for (row = firstRow; row < endRow; row += scale)
        for (column = firstColumn; column < endColumn; column += scale) {
            U8* pixel = buffer + yOffsets[row >> scaleBits] + xOffsets[column >> scaleBits];
            pixel[blueIndex] = pixel[1] = pixel[sourceIndex];
        }
}

Void JxrMonochromeExpansionReplicateUInt16AtScaledOffsets(U16* buffer, const size_t* xOffsets,
    const size_t* yOffsets, size_t firstRow, size_t endRow, size_t firstColumn,
    size_t endColumn, size_t scale, size_t scaleBits, size_t sourceIndex, size_t blueIndex)
{
    size_t row;
    size_t column;
    for (row = firstRow; row < endRow; row += scale)
        for (column = firstColumn; column < endColumn; column += scale) {
            U16* pixel = buffer + yOffsets[row >> scaleBits] + xOffsets[column >> scaleBits];
            pixel[blueIndex] = pixel[1] = pixel[sourceIndex];
        }
}

Void JxrMonochromeExpansionReplicateUInt32AtScaledOffsets(U32* buffer, const size_t* xOffsets,
    const size_t* yOffsets, size_t firstRow, size_t endRow, size_t firstColumn,
    size_t endColumn, size_t scale, size_t scaleBits, size_t sourceIndex, size_t blueIndex)
{
    size_t row;
    size_t column;
    for (row = firstRow; row < endRow; row += scale)
        for (column = firstColumn; column < endColumn; column += scale) {
            U32* pixel = buffer + yOffsets[row >> scaleBits] + xOffsets[column >> scaleBits];
            pixel[blueIndex] = pixel[1] = pixel[sourceIndex];
        }
}
