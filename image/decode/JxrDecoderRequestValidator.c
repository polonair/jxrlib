#include "JxrDecoderRequestValidator.h"
#include "JxrHeaderDecodePipeline.h"
#include "JxrStreamPositionScope.h"

Int JxrDecoderRequestValidatorReadInfo(CWMImageInfo* imageInfo,
    CWMIStrCodecParam* codecParameters)
{
    JxrStreamPositionScope positionScope;
    CCoreParameters dummyParameters;
    Bool readSucceeded;
    Bool restored;

    if (imageInfo == NULL || codecParameters == NULL ||
        !JxrStreamPositionScopeCapture(&positionScope, codecParameters->pWStream))
        return ICERR_ERROR;
    readSucceeded = JxrHeaderDecodePipelineRead(imageInfo, codecParameters,
        &dummyParameters);
    restored = JxrStreamPositionScopeRestore(&positionScope);
    return readSucceeded && restored ? ICERR_OK : ICERR_ERROR;
}

Int JxrDecoderRequestValidatorNormalize(CWMImageInfo* imageInfo,
    CWMIStrCodecParam* codecParameters, const CWMImageInfo* sourceImageInfo,
    const CWMIStrCodecParam* sourceCodecParameters)
{
    size_t thumbnailScale = 1;

    if (imageInfo == NULL || codecParameters == NULL || sourceImageInfo == NULL ||
        sourceCodecParameters == NULL) return ICERR_ERROR;
    imageInfo->bdBitDepth = sourceImageInfo->bdBitDepth;
    imageInfo->cWidth = sourceImageInfo->cWidth;
    imageInfo->cHeight = sourceImageInfo->cHeight;
    if (imageInfo->cWidth == 0 || imageInfo->cHeight == 0) return ICERR_ERROR;

    codecParameters->bVerbose = sourceCodecParameters->bVerbose;
    codecParameters->cbStream = sourceCodecParameters->cbStream;
    codecParameters->pWStream = sourceCodecParameters->pWStream;
    if (codecParameters->uAlphaMode > 1)
        codecParameters->uAlphaMode = sourceCodecParameters->uAlphaMode;

    if (codecParameters->cfColorFormat == NCOMPONENT)
        imageInfo->cfColorFormat = NCOMPONENT;
    if (codecParameters->cfColorFormat == CMYK && imageInfo->cfColorFormat != Y_ONLY &&
        imageInfo->cfColorFormat != CF_RGB)
        imageInfo->cfColorFormat = CMYK;
    if (codecParameters->cfColorFormat == YUV_422 && imageInfo->cfColorFormat == YUV_420)
        imageInfo->cfColorFormat = YUV_422;
    if (codecParameters->cfColorFormat == YUV_444 &&
        (imageInfo->cfColorFormat == YUV_422 || imageInfo->cfColorFormat == YUV_420))
        imageInfo->cfColorFormat = YUV_444;
    if (sourceImageInfo->cfColorFormat == CF_RGB && imageInfo->cfColorFormat != Y_ONLY &&
        imageInfo->cfColorFormat != NCOMPONENT)
        imageInfo->cfColorFormat = sourceImageInfo->cfColorFormat;
    if (sourceImageInfo->cfColorFormat == CF_RGBE)
        imageInfo->cfColorFormat = CF_RGBE;

    if (imageInfo->cThumbnailWidth == 0 || imageInfo->cThumbnailWidth > imageInfo->cWidth)
        imageInfo->cThumbnailWidth = imageInfo->cWidth;
    if (imageInfo->cThumbnailHeight == 0 || imageInfo->cThumbnailHeight > imageInfo->cHeight)
        imageInfo->cThumbnailHeight = imageInfo->cHeight;
    if ((imageInfo->cWidth + imageInfo->cThumbnailWidth - 1) /
            imageInfo->cThumbnailWidth !=
        (imageInfo->cHeight + imageInfo->cThumbnailHeight - 1) /
            imageInfo->cThumbnailHeight) {
        while ((imageInfo->cWidth + thumbnailScale - 1) / thumbnailScale >
                imageInfo->cThumbnailWidth &&
            (imageInfo->cHeight + thumbnailScale - 1) / thumbnailScale >
                imageInfo->cThumbnailHeight && (thumbnailScale << 1))
            thumbnailScale <<= 1;
    }
    else {
        thumbnailScale = (imageInfo->cWidth + imageInfo->cThumbnailWidth - 1) /
            imageInfo->cThumbnailWidth;
        if (thumbnailScale == 0) thumbnailScale = 1;
    }
    imageInfo->cThumbnailWidth = (imageInfo->cWidth + thumbnailScale - 1) /
        thumbnailScale;
    imageInfo->cThumbnailHeight = (imageInfo->cHeight + thumbnailScale - 1) /
        thumbnailScale;

    if (imageInfo->cROIHeight == 0 || imageInfo->cROIWidth == 0) {
        imageInfo->cROILeftX = imageInfo->cROITopY = 0;
        imageInfo->cROIWidth = imageInfo->cThumbnailWidth;
        imageInfo->cROIHeight = imageInfo->cThumbnailHeight;
    }
    if (imageInfo->cROILeftX >= imageInfo->cThumbnailWidth)
        imageInfo->cROILeftX = 0;
    if (imageInfo->cROITopY >= imageInfo->cThumbnailHeight)
        imageInfo->cROITopY = 0;
    if (imageInfo->cROILeftX + imageInfo->cROIWidth > imageInfo->cThumbnailWidth)
        imageInfo->cROIWidth = imageInfo->cThumbnailWidth - imageInfo->cROILeftX;
    if (imageInfo->cROITopY + imageInfo->cROIHeight > imageInfo->cThumbnailHeight)
        imageInfo->cROIHeight = imageInfo->cThumbnailHeight - imageInfo->cROITopY;
    return ICERR_OK;
}

Int JxrDecoderRequestValidatorValidateAndNormalize(CWMImageInfo* imageInfo,
    CWMIStrCodecParam* codecParameters)
{
    CWMImageInfo sourceImageInfo;
    CWMIStrCodecParam sourceCodecParameters;

    if (imageInfo == NULL || codecParameters == NULL) return ICERR_ERROR;
    sourceCodecParameters = *codecParameters;
    if (JxrDecoderRequestValidatorReadInfo(&sourceImageInfo, codecParameters) != ICERR_OK)
        return ICERR_ERROR;
    return JxrDecoderRequestValidatorNormalize(imageInfo, codecParameters,
        &sourceImageInfo, &sourceCodecParameters);
}
