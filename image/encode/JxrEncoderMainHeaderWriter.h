#ifndef JXR_ENCODER_MAIN_HEADER_WRITER_H
#define JXR_ENCODER_MAIN_HEADER_WRITER_H

#include "strcodec.h"

/* Immutable layout decisions for the JPEG XR main header. */
typedef struct JxrEncoderMainHeaderPlan {
    Bool usesAbbreviatedFields;
    Bool writesTiling;
    Bool writesWindowing;
    Bool usesAlternateOneBitDepth;
    Bool usesHardTileSubversion;
    U8 tileSizeBitCount;
} JxrEncoderMainHeaderPlan;

Void JxrEncoderMainHeaderPlanInitialize(
    JxrEncoderMainHeaderPlan* plan,
    size_t width,
    size_t height,
    U32 verticalSliceCountMinusOne,
    U32 horizontalSliceCountMinusOne,
    size_t extraPixelsTop,
    size_t extraPixelsLeft,
    size_t extraPixelsBottom,
    size_t extraPixelsRight,
    BITDEPTH_BITS bitDepth,
    Bool blackWhite,
    Bool usesHardTileBoundaries);

Int JxrEncoderMainHeaderWriterWrite(CWMImageStrCodec* codec);

#endif
