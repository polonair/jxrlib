#ifndef JXR_IMAGE_PLANE_QUANTIZER_HEADER_READER_H
#define JXR_IMAGE_PLANE_QUANTIZER_HEADER_READER_H

#include "JxrDecoderTileQuantizerSyntaxReader.h"

typedef struct JxrImagePlaneQuantizerHeader {
    U32 quantizerMode;
    Bool hasDc;
    Bool hasLp;
    Bool hasHp;
    U8 dcMode;
    U8 lpMode;
    U8 hpMode;
    U8 dcIndices[MAX_CHANNELS];
    U8 lpIndices[MAX_CHANNELS];
    U8 hpIndices[MAX_CHANNELS];
} JxrImagePlaneQuantizerHeader;

Bool JxrImagePlaneQuantizerHeaderReaderRead(SimpleBitIO* input, size_t channelCount,
    SUBBAND subband, JxrImagePlaneQuantizerHeader* result);

#endif
