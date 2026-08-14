#include "JxrInverseTransformPlanePlan.h"

Void JxrInverseTransformPlanePlanInitialize(
    JxrInverseTransformPlanePlan* plan,
    COLORFORMAT colorFormat,
    size_t fullResolutionChannelCount,
    size_t thumbnailScale)
{
    plan->transformsSamples = thumbnailScale < 16;
    plan->fullResolutionChannelCount = fullResolutionChannelCount;
    plan->chroma420ChannelCount = colorFormat == YUV_420 ? 2 : 0;
    plan->chroma422ChannelCount = colorFormat == YUV_422 ? 2 : 0;
}
