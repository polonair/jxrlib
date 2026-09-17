#ifndef JXR_TRANSCODE_PLANE_BUFFERS_H
#define JXR_TRANSCODE_PLANE_BUFFERS_H

#include "strcodec.h"

typedef struct JxrTranscodePlaneBuffers {
    PixelI* primaryCoefficients;
    size_t primaryCoefficientCount;
    PixelI* alphaCoefficients;
    size_t alphaCoefficientCount;
    Bool hasAlpha;
} JxrTranscodePlaneBuffers;

Bool JxrTranscodePlaneBuffersInitialize(JxrTranscodePlaneBuffers* buffers,
    PixelI* primaryCoefficients, size_t primaryCoefficientCount,
    PixelI* alphaCoefficients, size_t alphaCoefficientCount, Bool hasAlpha);

#endif
