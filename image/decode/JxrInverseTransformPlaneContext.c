#include "JxrInverseTransformPlaneContext.h"

Void JxrInverseTransformPlaneContextInitialize(
    JxrInverseTransformPlaneContext* context,
    PixelI* const* firstStagePlanes,
    PixelI* const* secondStagePlanes,
    Bool chroma,
    size_t channelIndex,
    const JxrInversePostProcessParameters* postProcessParameters,
    const JxrInverseHighPassParameters* highPassParameters)
{
    if (chroma) {
        JxrInverseTransformPlaneBuffersResolveChroma(
            &context->buffers, firstStagePlanes, secondStagePlanes, channelIndex);
    }
    else {
        JxrInverseTransformPlaneBuffersResolveFullResolution(
            &context->buffers, firstStagePlanes, secondStagePlanes, channelIndex);
    }
    context->channelIndex = channelIndex;
    context->lowPassQuantizer = postProcessParameters->lowPassQuantizers[channelIndex];
    context->directCurrentQuantizer = postProcessParameters->directCurrentQuantizers[channelIndex];
    context->highPassQuantizer = highPassParameters ?
        highPassParameters->quantizers[channelIndex] : JXR_INVERSE_DEFAULT_HIGH_PASS_QUANTIZER;
    context->isHighPassAbsent = highPassParameters ? highPassParameters->isAbsent : TRUE;
}
