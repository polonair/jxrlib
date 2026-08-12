#ifndef JXR_HEADER_VALIDATION_H
#define JXR_HEADER_VALIDATION_H

#include "JxrHeaderStateApplier.h"

typedef enum JxrHeaderValidationResult {
    JXR_HEADER_VALID = 0,
    JXR_HEADER_INVALID_QUANTIZER_MODE,
    JXR_HEADER_UNSUPPORTED_SOURCE_FORMAT
} JxrHeaderValidationResult;

JxrHeaderValidationResult JxrHeaderValidationValidateImagePlaneQuantizers(
    const CCoreParameters* coreParameters);
JxrHeaderValidationResult JxrHeaderValidationValidateSourceFormat(
    const CWMImageInfo* imageInfo, const CWMIStrCodecParam* codecParameters);

#endif
