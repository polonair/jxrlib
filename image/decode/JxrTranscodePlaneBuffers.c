#include "JxrTranscodePlaneBuffers.h"

Bool JxrTranscodePlaneBuffersInitialize(JxrTranscodePlaneBuffers* buffers,
    PixelI* primaryCoefficients, size_t primaryCoefficientCount,
    PixelI* alphaCoefficients, size_t alphaCoefficientCount, Bool hasAlpha)
{
    if (buffers == NULL || primaryCoefficients == NULL ||
        primaryCoefficientCount == 0 || (hasAlpha &&
            (alphaCoefficients == NULL || alphaCoefficientCount == 0)))
        return FALSE;
    buffers->primaryCoefficients = primaryCoefficients;
    buffers->primaryCoefficientCount = primaryCoefficientCount;
    buffers->alphaCoefficients = alphaCoefficients;
    buffers->alphaCoefficientCount = alphaCoefficientCount;
    buffers->hasAlpha = hasAlpha;
    return TRUE;
}
