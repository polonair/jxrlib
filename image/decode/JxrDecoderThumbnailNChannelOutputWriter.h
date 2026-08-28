#ifndef JXR_DECODER_THUMBNAIL_NCHANNEL_OUTPUT_WRITER_H
#define JXR_DECODER_THUMBNAIL_NCHANNEL_OUTPUT_WRITER_H

#include "JxrDecoderOutputRowPlan.h"

/* Writes a downsampled Y-only, alpha, YUV444, or N-channel macroblock row. */
Void JxrDecoderThumbnailNChannelOutputWriterWrite(CWMImageStrCodec* codec,
    const JxrDecoderOutputRowPlan* plan, PixelI multiplier, size_t shift);

#endif
