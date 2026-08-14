#include "JxrInverseTransformPlaneBuffers.h"

Void JxrInverseTransformPlaneBuffersResolveFullResolution(
    JxrInverseTransformPlaneBuffers* buffers,
    PixelI* const* firstStagePlanes,
    PixelI* const* secondStagePlanes,
    size_t channelIndex)
{
    buffers->firstStage = firstStagePlanes[channelIndex];
    buffers->secondStage = secondStagePlanes[channelIndex];
}

Void JxrInverseTransformPlaneBuffersResolveChroma(
    JxrInverseTransformPlaneBuffers* buffers,
    PixelI* const* firstStagePlanes,
    PixelI* const* secondStagePlanes,
    size_t chromaChannelIndex)
{
    JxrInverseTransformPlaneBuffersResolveFullResolution(
        buffers, firstStagePlanes, secondStagePlanes, chromaChannelIndex + 1);
}
