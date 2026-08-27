#ifndef JXR_DECODER_THUMBNAIL_ALPHA_OUTPUT_WRITER_H
#define JXR_DECODER_THUMBNAIL_ALPHA_OUTPUT_WRITER_H

#include "strcodec.h"

/* Writes downsampled secondary alpha samples for one decoded macroblock row. */
Int JxrDecoderThumbnailAlphaOutputWriterWrite(CWMImageStrCodec* codec,
    size_t thumbnailBits, PixelI multiplier, size_t shift);

#endif