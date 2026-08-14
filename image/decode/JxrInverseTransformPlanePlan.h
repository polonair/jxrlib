#ifndef JXR_INVERSE_TRANSFORM_PLANE_PLAN_H
#define JXR_INVERSE_TRANSFORM_PLANE_PLAN_H

#include "strcodec.h"

/* Immutable plane selection for one inverse-transform macroblock. */
typedef struct JxrInverseTransformPlanePlan {
    Bool transformsSamples;
    size_t fullResolutionChannelCount;
    size_t chroma420ChannelCount;
    size_t chroma422ChannelCount;
} JxrInverseTransformPlanePlan;

Void JxrInverseTransformPlanePlanInitialize(
    JxrInverseTransformPlanePlan* plan,
    COLORFORMAT colorFormat,
    size_t fullResolutionChannelCount,
    size_t thumbnailScale);

#endif
