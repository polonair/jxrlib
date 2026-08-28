#include "JxrDecoderOptimizationPolicy.h"

Void JxrDecoderOptimizationPolicyInitialize(JxrDecoderOptimizationPolicy* policy)
{
#if defined(WMP_OPT_SSE2)
    policy->nativeOptimizationAvailable = TRUE;
#else
    policy->nativeOptimizationAvailable = FALSE;
#endif
    policy->usesPortableBaseline = !policy->nativeOptimizationAvailable;
}
