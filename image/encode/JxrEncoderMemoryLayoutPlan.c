#include "JxrEncoderMemoryLayoutPlan.h"

Void JxrEncoderMemoryLayoutPlanInitialize(JxrEncoderMemoryLayoutPlan* plan,
    size_t channelBytes, size_t chromaBlockCount, size_t channelCount,
    size_t imageWidth, size_t codecStateBytes, size_t bitIoStateBytes,
    Bool isThirtyTwoBitBuild)
{
    size_t allocationPrefix;

    plan->macroblockCount = (imageWidth + 15) / 16;
    plan->fullResolutionMacroblockBytes = channelBytes * 16 * 16;
    plan->chromaMacroblockBytes = channelBytes * 16 * chromaBlockCount;
    plan->primaryMacroblockRowBytes = plan->fullResolutionMacroblockBytes +
        plan->chromaMacroblockBytes * (channelCount - 1);
    plan->allocationIsSafe = !(isThirtyTwoBitBuild &&
        (((plan->macroblockCount >> 15) * plan->primaryMacroblockRowBytes) &
            0xffff0000) != 0);
    plan->primaryMacroblockBufferBytes = plan->primaryMacroblockRowBytes *
        plan->macroblockCount * 2;
    plan->secondaryMacroblockBufferBytes = plan->fullResolutionMacroblockBytes *
        plan->macroblockCount * 2;

    allocationPrefix = codecStateBytes + (128 - 1) + (PACKETLENGTH * 4 - 1) +
        (PACKETLENGTH * 2) + bitIoStateBytes;
    plan->primaryAllocationBytes = allocationPrefix + plan->primaryMacroblockBufferBytes;
    plan->secondaryAllocationBytes = codecStateBytes + (128 - 1) +
        plan->secondaryMacroblockBufferBytes;
}
