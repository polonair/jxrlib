#include "JxrForwardTransformPlanePlan.h"

Void JxrForwardTransformPlanePlanInitialize(
    JxrForwardTransformPlanePlan* plan,
    COLORFORMAT colorFormat,
    size_t fullResolutionChannelCount)
{
    plan->fullResolutionChannelCount = fullResolutionChannelCount;
    plan->chroma420ChannelCount = colorFormat == YUV_420 ? 2 : 0;
    plan->chroma422ChannelCount = colorFormat == YUV_422 ? 2 : 0;
}
