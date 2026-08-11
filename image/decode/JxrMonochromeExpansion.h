#ifndef JXR_MONOCHROME_EXPANSION_H
#define JXR_MONOCHROME_EXPANSION_H

#include "windowsmediaphoto.h"

Void JxrMonochromeExpansionReplicateByte(U8* buffer, size_t strideBytes,
    size_t width, size_t height, size_t componentsPerPixel);
Void JxrMonochromeExpansionReplicateUInt16(U16* buffer, size_t strideBytes,
    size_t width, size_t height, size_t componentsPerPixel);
Void JxrMonochromeExpansionReplicateUInt32(U32* buffer, size_t strideBytes,
    size_t width, size_t height, size_t componentsPerPixel);

#endif
