#ifndef JXR_DECODER_INITIALIZATION_PIPELINE_H
#define JXR_DECODER_INITIALIZATION_PIPELINE_H

#include "JxrHeaderDecodePipeline.h"

typedef struct JxrDecoderInitializationPipeline {
    CWMImageStrCodec* primaryCodec;
    CWMImageStrCodec* secondaryCodec;
} JxrDecoderInitializationPipeline;

Void JxrDecoderInitializationPipelineInit(JxrDecoderInitializationPipeline* pipeline,
    CWMImageStrCodec* primaryCodec, CWMImageStrCodec* secondaryCodec);
Int JxrDecoderInitializationPipelineRun(JxrDecoderInitializationPipeline* pipeline);

#endif
