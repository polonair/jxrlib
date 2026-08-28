#ifndef JXR_DECODER_STANDARD_COLOR_OUTPUT_WRITER_H
#define JXR_DECODER_STANDARD_COLOR_OUTPUT_WRITER_H

#include "JxrDecoderOutputRowPlan.h"

/* Writes all non-alpha samples for one full-resolution macroblock row. */
Void JxrDecoderStandardColorOutputWriterWrite(CWMImageStrCodec* codec,
    const JxrDecoderOutputRowPlan* plan, size_t shift);

#endif
