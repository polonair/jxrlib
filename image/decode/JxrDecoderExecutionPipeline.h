#ifndef JXR_DECODER_EXECUTION_PIPELINE_H
#define JXR_DECODER_EXECUTION_PIPELINE_H

#include "strcodec.h"

Int JxrDecoderExecutionPipelineRun(CWMImageStrCodec* codec, size_t macroblockRowCount,
    Bool usesLegacyLoadCallback
#ifdef REENTRANT_MODE
    , size_t* decodedLines
#endif
    );

#endif
