#include "JxrDecoderNativeOverridePlan.h"
#include "JxrDecoderOptimizationEligibility.h"
#include "decode.h"

Void JxrDecoderNativeOverridePlanInitialize(JxrDecoderNativeOverridePlan* plan,
    const CWMImageStrCodec* codec, const JxrDecoderOptimizationPolicy* policy)
{
    plan->useRgb24Output = policy->nativeOptimizationAvailable &&
        JxrDecoderOptimizationEligibilityCanUseRgb24Output(codec);
    plan->useLossyRgb24Output = plan->useRgb24Output &&
        (codec->m_param.bScaledArith || codec->WMISCP.olOverlap != OL_NONE);
    plan->useYuv444CenterTransform = policy->nativeOptimizationAvailable &&
        JxrDecoderOptimizationEligibilityCanUseYuv444CenterTransform(codec);
}
