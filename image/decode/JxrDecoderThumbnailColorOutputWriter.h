#ifndef JXR_DECODER_THUMBNAIL_COLOR_OUTPUT_WRITER_H
#define JXR_DECODER_THUMBNAIL_COLOR_OUTPUT_WRITER_H

#include "JxrDecoderOutputRowPlan.h"

/* Writes all non-alpha color samples for one downsampled macroblock row. */
Void JxrDecoderThumbnailColorOutputWriterWrite(CWMImageStrCodec* codec,
    const JxrDecoderOutputRowPlan* outputPlan,
    const JxrDecoderOutputRowPlan* nChannelPlan,
    PixelI multiplier, size_t lumaShift, size_t chromaShift);

#endif
