#ifndef JXR_FORWARD_TRANSFORM_PLANE_PLAN_H
#define JXR_FORWARD_TRANSFORM_PLANE_PLAN_H

#include "strcodec.h"

/* Immutable plane selection for one forward-transform macroblock. */
typedef struct JxrForwardTransformPlanePlan {
    size_t fullResolutionChannelCount;
    size_t chroma420ChannelCount;
    size_t chroma422ChannelCount;
} JxrForwardTransformPlanePlan;

Void JxrForwardTransformPlanePlanInitialize(
    JxrForwardTransformPlanePlan* plan,
    COLORFORMAT colorFormat,
    size_t fullResolutionChannelCount);

#endif
