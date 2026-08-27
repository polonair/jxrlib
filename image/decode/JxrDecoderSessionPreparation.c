#include "JxrDecoderSessionPreparation.h"
#include "JxrHeaderDecodePipeline.h"
#include <string.h>

Int JxrDecoderSessionPreparationPrepare(CWMImageInfo* imageInfo,
    CWMIStrCodecParam* codecParameters, JxrDecoderSessionPreparation* preparation)
{
    CWMImageStrCodec* templateCodec;

    if (imageInfo == NULL || codecParameters == NULL || preparation == NULL)
        return ICERR_ERROR;
    memset(preparation, 0, sizeof(*preparation));
    if (WMPhotoValidate(imageInfo, codecParameters) != ICERR_OK ||
        codecParameters->sbSubband == SB_ISOLATED) return ICERR_ERROR;

    templateCodec = &preparation->templateCodec;
    templateCodec->WMISCP.pWStream = codecParameters->pWStream;
    if (!JxrHeaderDecodePipelineRead(&templateCodec->WMII, &templateCodec->WMISCP,
        &templateCodec->m_param)) return ICERR_ERROR;

    preparation->usesHardTileBoundaries = templateCodec->WMISCP.bUseHardTileBoundaries;
    preparation->isLossyTranscoding = templateCodec->WMII.cfColorFormat == CMYK &&
        imageInfo->cfColorFormat == CF_RGB;
    if (codecParameters->cfColorFormat != CMYK && imageInfo->cfColorFormat == CMYK)
        return ICERR_ERROR;

    templateCodec->WMISCP = *codecParameters;
    templateCodec->WMII = *imageInfo;
    templateCodec->WMII.cWidth += templateCodec->m_param.cExtraPixelsLeft +
        templateCodec->m_param.cExtraPixelsRight;
    templateCodec->WMII.cHeight += templateCodec->m_param.cExtraPixelsTop +
        templateCodec->m_param.cExtraPixelsBottom;
    imageInfo->cROILeftX += templateCodec->m_param.cExtraPixelsLeft;
    imageInfo->cROITopY += templateCodec->m_param.cExtraPixelsTop;
    return ICERR_OK;
}
