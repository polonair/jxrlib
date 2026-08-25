#ifndef JXR_DECODER_INITIALIZATION_PIPELINE_H
#define JXR_DECODER_INITIALIZATION_PIPELINE_H

#include "JxrHeaderDecodePipeline.h"

typedef Int (*JxrDecoderInitializeStage)(CWMImageStrCodec* codec);

typedef struct JxrDecoderInitializationPipeline {
    CWMImageStrCodec* primaryCodec;
    CWMImageStrCodec* secondaryCodec;
    JxrDecoderInitializeStage initializeIo;
} JxrDecoderInitializationPipeline;

Void JxrDecoderInitializationPipelineInit(JxrDecoderInitializationPipeline* pipeline,
    CWMImageStrCodec* primaryCodec, CWMImageStrCodec* secondaryCodec,
    JxrDecoderInitializeStage initializeIo);
Int JxrDecoderInitializationPipelineRun(JxrDecoderInitializationPipeline* pipeline);

#endif
