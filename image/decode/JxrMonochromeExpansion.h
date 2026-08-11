#ifndef JXR_MONOCHROME_EXPANSION_H
#define JXR_MONOCHROME_EXPANSION_H

#include "windowsmediaphoto.h"

Void JxrMonochromeExpansionReplicateByte(U8* buffer, size_t strideBytes,
    size_t width, size_t height, size_t componentsPerPixel);
Void JxrMonochromeExpansionReplicateUInt16(U16* buffer, size_t strideBytes,
    size_t width, size_t height, size_t componentsPerPixel);
Void JxrMonochromeExpansionReplicateUInt32(U32* buffer, size_t strideBytes,
    size_t width, size_t height, size_t componentsPerPixel);
Void JxrMonochromeExpansionReplicateByteAtOffsets(U8* buffer, const size_t* xOffsets,
    const size_t* yOffsets, size_t firstRow, size_t endRow, size_t firstColumn,
    size_t endColumn);
Void JxrMonochromeExpansionReplicateUInt16AtOffsets(U16* buffer, const size_t* xOffsets,
    const size_t* yOffsets, size_t firstRow, size_t endRow, size_t firstColumn,
    size_t endColumn);
Void JxrMonochromeExpansionReplicateUInt32AtOffsets(U32* buffer, const size_t* xOffsets,
    const size_t* yOffsets, size_t firstRow, size_t endRow, size_t firstColumn,
    size_t endColumn);

#endif
