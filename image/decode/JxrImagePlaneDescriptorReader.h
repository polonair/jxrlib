#ifndef JXR_IMAGE_PLANE_DESCRIPTOR_READER_H
#define JXR_IMAGE_PLANE_DESCRIPTOR_READER_H

#include "JxrImagePlaneQuantizerHeaderReader.h"

typedef struct JxrImagePlaneDescriptor {
    COLORFORMAT colorFormat;
    Bool scaledArithmetic;
    SUBBAND subband;
    size_t channelCount;
    Bool hasChromaCenteringX;
    Bool hasChromaCenteringY;
    U8 chromaCenteringX;
    U8 chromaCenteringY;
    Bool hasSampleConversion;
    U8 mantissaOrShift;
    I8 exponentBias;
} JxrImagePlaneDescriptor;

Bool JxrImagePlaneDescriptorReaderRead(SimpleBitIO* input, BITDEPTH_BITS bitDepth,
    JxrImagePlaneDescriptor* result);

#endif
