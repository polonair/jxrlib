#ifndef JXR_DECODER_ALPHA_OUTPUT_WRITER_H
#define JXR_DECODER_ALPHA_OUTPUT_WRITER_H

#include "strcodec.h"

/* Writes the secondary alpha plane for one decoded macroblock row. */
Int JxrDecoderAlphaOutputWriterWrite(CWMImageStrCodec* codec);

#endif