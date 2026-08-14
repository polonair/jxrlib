#ifndef JXR_INVERSE_HIGH_PASS_PARAMETERS_H
#define JXR_INVERSE_HIGH_PASS_PARAMETERS_H

#include "strcodec.h"

#define JXR_INVERSE_DEFAULT_HIGH_PASS_QUANTIZER 255

/* HP subband availability and quantizers used by normal inverse overlap. */
typedef struct JxrInverseHighPassParameters {
    Bool isAbsent;
    Int quantizers[MAX_CHANNELS];
} JxrInverseHighPassParameters;

Void JxrInverseHighPassParametersInitialize(
    JxrInverseHighPassParameters* parameters,
    SUBBAND subband,
    size_t channelCount,
    CWMIQuantizer* const* highPassQuantizers,
    U8 highPassQuantizerIndex);

#endif
