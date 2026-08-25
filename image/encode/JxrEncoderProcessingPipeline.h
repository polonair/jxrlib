#ifndef JXR_ENCODER_PROCESSING_PIPELINE_H
#define JXR_ENCODER_PROCESSING_PIPELINE_H

#include "strcodec.h"

typedef struct JxrEncoderProcessingPipelinePlan {
    Bool usesLegacyLoadCallback;
} JxrEncoderProcessingPipelinePlan;

Void JxrEncoderProcessingPipelinePlanInitialize(JxrEncoderProcessingPipelinePlan* plan,
    Bool hasOptimizedLoadOverride);
Int JxrEncoderProcessingPipelineLoadInput(CWMImageStrCodec* codec);

#endif
