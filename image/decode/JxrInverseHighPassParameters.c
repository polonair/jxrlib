#include "JxrInverseHighPassParameters.h"

#include <string.h>

Void JxrInverseHighPassParametersInitialize(
    JxrInverseHighPassParameters* parameters,
    SUBBAND subband,
    size_t channelCount,
    CWMIQuantizer* const* highPassQuantizers,
    U8 highPassQuantizerIndex)
{
    size_t channel;

    memset(parameters, 0, sizeof(*parameters));
    parameters->isAbsent = subband == SB_NO_HIGHPASS || subband == SB_DC_ONLY;
    for (channel = 0; channel < MAX_CHANNELS; ++channel) {
        parameters->quantizers[channel] = JXR_INVERSE_DEFAULT_HIGH_PASS_QUANTIZER;
    }
    if (parameters->isAbsent) return;

    for (channel = 0; channel < channelCount; ++channel) {
        parameters->quantizers[channel] =
            highPassQuantizers[channel][highPassQuantizerIndex].iQP;
    }
}
