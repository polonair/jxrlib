#include "JxrEncoderProcessingPipeline.h"
#include "JxrEncoderInputRowProcessor.h"

Void JxrEncoderProcessingPipelinePlanInitialize(JxrEncoderProcessingPipelinePlan* plan,
    Bool hasOptimizedLoadOverride)
{
    plan->usesLegacyLoadCallback = hasOptimizedLoadOverride;
}

Int JxrEncoderProcessingPipelineLoadInput(CWMImageStrCodec* codec)
{
    return JxrEncoderInputRowProcessorProcess(codec);
}
