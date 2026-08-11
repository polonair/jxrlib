#ifndef JXR_MAIN_HEADER_READER_H
#define JXR_MAIN_HEADER_READER_H

#include "JxrImagePlaneDescriptorReader.h"

typedef struct JxrMainHeaderDescriptor {
    U8 codecVersion;
    U8 codecSubVersion;
    Bool useHardTileBoundaries;
    BITSTREAMFORMAT bitstreamFormat;
    ORIENTATION orientation;
    Bool hasIndexTable;
    OVERLAP overlap;
    BITDEPTH codedBitDepth;
    Bool trimFlexbits;
    Bool redBlueSwapped;
    Bool hasAlphaChannel;
    COLORFORMAT sourceColorFormat;
    BITDEPTH_BITS sourceBitDepth;
    Bool blackWhite;
    size_t width;
    size_t height;
    size_t extraPixelsTop;
    size_t extraPixelsLeft;
    size_t extraPixelsBottom;
    size_t extraPixelsRight;
    U32 verticalSliceCountMinusOne;
    U32 horizontalSliceCountMinusOne;
    U32 tileX[MAX_TILES];
    U32 tileY[MAX_TILES];
} JxrMainHeaderDescriptor;

Bool JxrMainHeaderReaderRead(SimpleBitIO* input, JxrMainHeaderDescriptor* result);

#endif
