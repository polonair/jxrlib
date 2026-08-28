#ifndef JXR_ENCODER_REQUEST_VALIDATOR_H
#define JXR_ENCODER_REQUEST_VALIDATOR_H

#include "strcodec.h"

/* Produces uniform tile sizes when an input tile layout cannot be used. */
U32 JxrEncoderRequestValidatorSetUniformTiling(U32* tileSizes,
    U32 tileCount, U32 macroblockCount);

/* Normalizes tile sizes to zero-based cumulative macroblock boundaries. */
U32 JxrEncoderRequestValidatorNormalizeTiling(U32* tileBoundaries,
    U32 tileCount, U32 macroblockCount);

/* Validates an encoder request and normalizes its mutable parameters. */
Int JxrEncoderRequestValidatorValidateAndNormalize(CWMImageInfo* imageInfo,
    CWMIStrCodecParam* codecParameters);

#endif
