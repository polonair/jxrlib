#ifndef JXR_DECODER_SESSION_EXECUTOR_H
#define JXR_DECODER_SESSION_EXECUTOR_H

#include "strcodec.h"

/* Decodes a prepared session into its caller-owned output buffer. */
Int JxrDecoderSessionExecutorExecute(CWMImageStrCodec* codec,
    const CWMImageBufferInfo* outputBuffer
#ifdef REENTRANT_MODE
    , size_t* decodedLines
#endif
    );

#endif
