#include "JxrDecoderOutputPipeline.h"

/* The output routine remains in strdec.c until its color conversion helpers move with it. */
Int outputMBRow(CWMImageStrCodec* codec);

Void JxrDecoderOutputPipelinePlanInitialize(JxrDecoderOutputPipelinePlan* plan,
    Bool hasOptimizedLoadOverride)
{
    plan->usesLegacyLoadCallback = hasOptimizedLoadOverride;
}

Int JxrDecoderOutputPipelineWriteStandardRow(CWMImageStrCodec* codec)
{
    return outputMBRow(codec);
}
