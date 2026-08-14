#include "JxrInversePostProcessParameters.h"

#include <string.h>

Void JxrInversePostProcessParametersInitialize(
    JxrInversePostProcessParameters* parameters,
    Int postProcessStrength,
    OVERLAP overlap,
    size_t channelCount,
    CWMIQuantizer* const* lowPassQuantizers,
    CWMIQuantizer* const* directCurrentQuantizers,
    U8 lowPassQuantizerIndex)
{
    size_t channel;
    Int strengthScale;
    Int overlapScale;

    memset(parameters, 0, sizeof(*parameters));
    parameters->enabled = postProcessStrength > 0;
    if (!parameters->enabled) return;

    strengthScale = 1 << postProcessStrength;
    overlapScale = overlap == OL_NONE ? 2 : 1;
    for (channel = 0; channel < channelCount; ++channel) {
        parameters->lowPassQuantizers[channel] =
            lowPassQuantizers[channel][lowPassQuantizerIndex].iQP * strengthScale * overlapScale;
        parameters->directCurrentQuantizers[channel] =
            directCurrentQuantizers[channel][0].iQP * strengthScale;
    }
}
