#include "JxrSecondaryPlaneMemoryLayoutPlan.h"

Void JxrSecondaryPlaneMemoryLayoutPlanInitialize(
    JxrSecondaryPlaneMemoryLayoutPlan* plan, size_t channelBytes,
    size_t macroblockCount, size_t codecStateBytes)
{
    plan->macroblockStride = channelBytes * 16 * 16;
    plan->macroblockBufferBytes = plan->macroblockStride * macroblockCount * 2;
    plan->allocationBytes = codecStateBytes + (128 - 1) +
        plan->macroblockBufferBytes;
}
