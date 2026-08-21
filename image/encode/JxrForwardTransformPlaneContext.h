#ifndef JXR_FORWARD_TRANSFORM_PLANE_CONTEXT_H
#define JXR_FORWARD_TRANSFORM_PLANE_CONTEXT_H

#include "strcodec.h"

/* Buffers and prediction state resolved for one forward-transform plane. */
typedef struct JxrForwardTransformPlaneContext {
    PixelI* firstStage;
    PixelI* secondStage;
    PixelI* predictionBefore;
    PixelI* predictionAfter;
    size_t channelIndex;
    Bool isChroma;
} JxrForwardTransformPlaneContext;

Void JxrForwardTransformPlaneContextInitializeFullResolution(
    JxrForwardTransformPlaneContext* context,
    PixelI* const* firstStagePlanes,
    PixelI* const* secondStagePlanes,
    size_t channelIndex);

Void JxrForwardTransformPlaneContextInitializeChroma(
    JxrForwardTransformPlaneContext* context,
    PixelI* const* firstStagePlanes,
    PixelI* const* secondStagePlanes,
    PixelI predictionBefore[MAX_CHANNELS][2],
    PixelI predictionAfter[MAX_CHANNELS][2],
    size_t channelIndex);

#endif
