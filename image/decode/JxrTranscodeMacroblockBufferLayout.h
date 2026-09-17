#ifndef JXR_TRANSCODE_MACROBLOCK_BUFFER_LAYOUT_H
#define JXR_TRANSCODE_MACROBLOCK_BUFFER_LAYOUT_H

#include "strcodec.h"

/* Managed-port shape: one coefficient array plus an offset for every channel. */
typedef struct JxrTranscodeMacroblockBufferLayout {
    size_t coefficientCount;
    size_t channelCount;
    size_t channelOffsets[MAX_CHANNELS];
} JxrTranscodeMacroblockBufferLayout;

Bool JxrTranscodeMacroblockBufferLayoutInitialize(COLORFORMAT colorFormat,
    size_t channelCount, JxrTranscodeMacroblockBufferLayout* layout);

Bool JxrTranscodeMacroblockBufferLayoutBindCompatibilityPointers(
    CWMImageStrCodec* codec, PixelI* coefficientBuffer,
    const JxrTranscodeMacroblockBufferLayout* layout);

#endif
