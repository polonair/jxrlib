#ifndef JXR_ENCODER_SESSION_ENCODER_H
#define JXR_ENCODER_SESSION_ENCODER_H

#include "strcodec.h"

/* Load and encode one input macroblock row for an initialized session. */
Int JxrEncoderSessionEncoderEncodeRow(CWMImageStrCodec* codec,
    const CWMImageBufferInfo* bufferInfo);

#endif
