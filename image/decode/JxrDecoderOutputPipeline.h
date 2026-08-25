#ifndef JXR_DECODER_OUTPUT_PIPELINE_H
#define JXR_DECODER_OUTPUT_PIPELINE_H

#include "strcodec.h"

typedef struct JxrDecoderOutputPipelinePlan {
    Bool usesLegacyLoadCallback;
} JxrDecoderOutputPipelinePlan;

Void JxrDecoderOutputPipelinePlanInitialize(JxrDecoderOutputPipelinePlan* plan,
    Bool hasOptimizedLoadOverride);
Int JxrDecoderOutputPipelineWriteStandardRow(CWMImageStrCodec* codec);

#endif
