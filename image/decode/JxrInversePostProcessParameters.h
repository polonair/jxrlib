#ifndef JXR_INVERSE_POST_PROCESS_PARAMETERS_H
#define JXR_INVERSE_POST_PROCESS_PARAMETERS_H

#include "strcodec.h"

/* Precomputed post-processing thresholds for one inverse-transform macroblock. */
typedef struct JxrInversePostProcessParameters {
    Bool enabled;
    Int lowPassQuantizers[MAX_CHANNELS];
    Int directCurrentQuantizers[MAX_CHANNELS];
} JxrInversePostProcessParameters;

Void JxrInversePostProcessParametersInitialize(
    JxrInversePostProcessParameters* parameters,
    Int postProcessStrength,
    OVERLAP overlap,
    size_t channelCount,
    CWMIQuantizer* const* lowPassQuantizers,
    CWMIQuantizer* const* directCurrentQuantizers,
    U8 lowPassQuantizerIndex);

#endif
