#ifndef JXR_SECONDARY_PLANE_MEMORY_LAYOUT_PLAN_H
#define JXR_SECONDARY_PLANE_MEMORY_LAYOUT_PLAN_H

#include "strcodec.h"

/* Size-only plan for the secondary alpha-plane allocation. */
typedef struct JxrSecondaryPlaneMemoryLayoutPlan {
    size_t macroblockStride;
    size_t macroblockBufferBytes;
    size_t allocationBytes;
} JxrSecondaryPlaneMemoryLayoutPlan;

Void JxrSecondaryPlaneMemoryLayoutPlanInitialize(
    JxrSecondaryPlaneMemoryLayoutPlan* plan, size_t channelBytes,
    size_t macroblockCount, size_t codecStateBytes);

#endif
