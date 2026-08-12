#include "JxrHeaderValidation.h"

JxrHeaderValidationResult JxrHeaderValidationValidateImagePlaneQuantizers(
    const CCoreParameters* coreParameters)
{
    if (coreParameters == NULL || (coreParameters->uQPMode & 0x600) == 0)
        return JXR_HEADER_INVALID_QUANTIZER_MODE;
    return JXR_HEADER_VALID;
}

JxrHeaderValidationResult JxrHeaderValidationValidateSourceFormat(
    const CWMImageInfo* imageInfo, const CWMIStrCodecParam* codecParameters)
{
    if (imageInfo == NULL || codecParameters == NULL)
        return JXR_HEADER_UNSUPPORTED_SOURCE_FORMAT;
    if ((imageInfo->bdBitDepth == BD_5 || imageInfo->bdBitDepth == BD_10 ||
        imageInfo->bdBitDepth == BD_565) && codecParameters->cfColorFormat != YUV_444 &&
        codecParameters->cfColorFormat != YUV_422 && codecParameters->cfColorFormat != YUV_420 &&
        codecParameters->cfColorFormat != Y_ONLY)
        return JXR_HEADER_UNSUPPORTED_SOURCE_FORMAT;
    return JXR_HEADER_VALID;
}
