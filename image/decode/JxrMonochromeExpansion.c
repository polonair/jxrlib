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
