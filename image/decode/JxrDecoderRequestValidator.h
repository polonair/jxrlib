#ifndef JXR_DECODER_REQUEST_VALIDATOR_H
#define JXR_DECODER_REQUEST_VALIDATOR_H

#include "strcodec.h"

/* Reads image metadata without advancing the caller's stream position. */
Int JxrDecoderRequestValidatorReadInfo(CWMImageInfo* imageInfo,
    CWMIStrCodecParam* codecParameters);

/* Applies source metadata to an output request and normalizes thumbnail and ROI. */
Int JxrDecoderRequestValidatorNormalize(CWMImageInfo* imageInfo,
    CWMIStrCodecParam* codecParameters, const CWMImageInfo* sourceImageInfo,
    const CWMIStrCodecParam* sourceCodecParameters);

/* Reads source metadata and normalizes a decoder request. */
Int JxrDecoderRequestValidatorValidateAndNormalize(CWMImageInfo* imageInfo,
    CWMIStrCodecParam* codecParameters);

#endif
