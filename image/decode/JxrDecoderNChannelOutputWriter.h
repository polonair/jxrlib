#ifndef JXR_DECODER_NCHANNEL_OUTPUT_WRITER_H
#define JXR_DECODER_NCHANNEL_OUTPUT_WRITER_H

#include "strcodec.h"

/* Writes one N-channel macroblock region to the caller-owned output buffer. */
Void JxrDecoderNChannelOutputWriterWrite(CWMImageStrCodec* codec,
    size_t firstRow, size_t firstColumn, size_t width, size_t height,
    size_t shift, PixelI bias);

#endif