#include "JxrForwardTransformPlaneContext.h"

Void JxrForwardTransformPlaneContextInitializeFullResolution(
    JxrForwardTransformPlaneContext* context,
    PixelI* const* firstStagePlanes,
    PixelI* const* secondStagePlanes,
    size_t channelIndex)
{
    context->firstStage = firstStagePlanes[channelIndex];
    context->secondStage = secondStagePlanes[channelIndex];
    context->predictionBefore = NULL;
    context->predictionAfter = NULL;
    context->channelIndex = channelIndex;
    context->isChroma = FALSE;
}

Void JxrForwardTransformPlaneContextInitializeChroma(
    JxrForwardTransformPlaneContext* context,
    PixelI* const* firstStagePlanes,
    PixelI* const* secondStagePlanes,
    PixelI predictionBefore[MAX_CHANNELS][2],
    PixelI predictionAfter[MAX_CHANNELS][2],
    size_t channelIndex)
{
    context->firstStage = firstStagePlanes[1 + channelIndex];
    context->secondStage = secondStagePlanes[1 + channelIndex];
    context->predictionBefore = predictionBefore[channelIndex];
    context->predictionAfter = predictionAfter[channelIndex];
    context->channelIndex = channelIndex;
    context->isChroma = TRUE;
}
