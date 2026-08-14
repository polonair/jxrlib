#ifndef JXR_INVERSE_TRANSFORM_PLANE_CONTEXT_H
#define JXR_INVERSE_TRANSFORM_PLANE_CONTEXT_H

#include "JxrInverseHighPassParameters.h"
#include "JxrInversePostProcessParameters.h"
#include "JxrInverseTransformPlaneBuffers.h"

typedef struct JxrInverseTransformPlaneContext {
    JxrInverseTransformPlaneBuffers buffers;
    size_t channelIndex;
    Int lowPassQuantizer;
    Int directCurrentQuantizer;
    Int highPassQuantizer;
    Bool isHighPassAbsent;
} JxrInverseTransformPlaneContext;

Void JxrInverseTransformPlaneContextInitialize(
    JxrInverseTransformPlaneContext* context,
    PixelI* const* firstStagePlanes,
    PixelI* const* secondStagePlanes,
    Bool chroma,
    size_t channelIndex,
    const JxrInversePostProcessParameters* postProcessParameters,
    const JxrInverseHighPassParameters* highPassParameters);

#endif
