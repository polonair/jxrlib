#include "JxrDecoderMemoryLayoutPlan.h"

Void JxrDecoderMemoryLayoutPlanInitialize(JxrDecoderMemoryLayoutPlan* plan,
    BITDEPTH bitDepth, COLORFORMAT colorFormat, size_t channelCount,
    size_t imageWidth, size_t codecStateBytes, size_t decoderParametersBytes,
    size_t bitIoStateBytes, Bool isThirtyTwoBitBuild)
{
    static const size_t channelBytesByBitDepth[BD_MAX] = {2, 4};
    size_t twoRowBytes;
    size_t allocationPrefix;

    plan->channelBytes = channelBytesByBitDepth[bitDepth];
    plan->chromaBlockCount = cblkChromas[colorFormat];
    plan->macroblockCount = (imageWidth + 15) / 16;
    plan->fullResolutionMacroblockBytes = plan->channelBytes * 16 * 16;
    plan->chromaMacroblockBytes = plan->channelBytes * 16 *
        plan->chromaBlockCount;
    plan->primaryMacroblockRowBytes = plan->fullResolutionMacroblockBytes +
        plan->chromaMacroblockBytes * (channelCount - 1);
    twoRowBytes = plan->primaryMacroblockRowBytes * 2;
    plan->allocationIsSafe = !(isThirtyTwoBitBuild &&
        ((twoRowBytes * (plan->macroblockCount >> 16)) & 0xffffc000));
    plan->primaryMacroblockBufferBytes = twoRowBytes * plan->macroblockCount;

    allocationPrefix = codecStateBytes + (128 - 1) +
        decoderParametersBytes + (PACKETLENGTH * 4 - 1) +
        (PACKETLENGTH * 2) + bitIoStateBytes;
    plan->allocationBytes = allocationPrefix + plan->primaryMacroblockBufferBytes;
}
