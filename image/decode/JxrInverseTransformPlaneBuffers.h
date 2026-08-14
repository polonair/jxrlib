#ifndef JXR_INVERSE_TRANSFORM_PLANE_BUFFERS_H
#define JXR_INVERSE_TRANSFORM_PLANE_BUFFERS_H

#include "strcodec.h"

/* Paired coefficient buffers for one inverse-transform plane. */
typedef struct JxrInverseTransformPlaneBuffers {
    PixelI* firstStage;
    PixelI* secondStage;
} JxrInverseTransformPlaneBuffers;

Void JxrInverseTransformPlaneBuffersResolveFullResolution(
    JxrInverseTransformPlaneBuffers* buffers,
    PixelI* const* firstStagePlanes,
    PixelI* const* secondStagePlanes,
    size_t channelIndex);
Void JxrInverseTransformPlaneBuffersResolveChroma(
    JxrInverseTransformPlaneBuffers* buffers,
    PixelI* const* firstStagePlanes,
    PixelI* const* secondStagePlanes,
    size_t chromaChannelIndex);

#endif
