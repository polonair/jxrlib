#include "JxrTranscodeRoiInitializer.h"
#include "JxrTranscodeRoiGeometry.h"
#include "JxrTranscodeRoiTileLayout.h"

static Void JxrTranscodeRoiInitializerSwapSize(size_t* left, size_t* right)
{
    size_t temporary = *left;
    *left = *right;
    *right = temporary;
}

static Int JxrTranscodeRoiInitializerApply(CWMImageInfo* imageInfo,
    CCoreParameters* coreParameters, CWMIStrCodecParam* streamParameters,
    CWMTranscodingParam* parameters)
{
    JxrTranscodeOrientationState orientation;
    JxrTranscodeRoiGeometryRequest request;
    JxrTranscodeRoiGeometryResult geometry;
    JxrTranscodeRoiTileLayout tileLayout;
    size_t boundaryIndex;

    JxrTranscodeOrientationStateInit(&orientation, parameters->oOrientation);
    memset(&request, 0, sizeof(request));
    request.imageWidth = imageInfo->cWidth;
    request.imageHeight = imageInfo->cHeight;
    request.extraLeft = coreParameters->cExtraPixelsLeft;
    request.extraTop = coreParameters->cExtraPixelsTop;
    request.extraRight = coreParameters->cExtraPixelsRight;
    request.extraBottom = coreParameters->cExtraPixelsBottom;
    request.requestedLeft = parameters->cLeftX;
    request.requestedTop = parameters->cTopY;
    request.requestedWidth = parameters->cWidth;
    request.requestedHeight = parameters->cHeight;
    request.overlap = streamParameters->olOverlap;
    request.ignoreOverlap = parameters->bIgnoreOverlap;
    if (JxrTranscodeRoiGeometryCalculate(&request, &geometry) == FALSE ||
        JxrTranscodeRoiTileLayoutInitialize(&tileLayout, streamParameters->uiTileX,
            (size_t)streamParameters->cNumOfSliceMinus1V + 1,
            streamParameters->uiTileY,
            (size_t)streamParameters->cNumOfSliceMinus1H + 1) == FALSE ||
        JxrTranscodeRoiTileLayoutApply(&tileLayout, geometry.macroblockLeft,
            geometry.macroblockRight, geometry.macroblockTop,
            geometry.macroblockBottom, &orientation) == FALSE)
        return ICERR_ERROR;

    coreParameters->cExtraPixelsLeft = geometry.extraLeft;
    coreParameters->cExtraPixelsTop = geometry.extraTop;
    coreParameters->cExtraPixelsRight = geometry.extraRight;
    coreParameters->cExtraPixelsBottom = geometry.extraBottom;
    JxrTranscodeRoiTileLayoutOrientExtraPixels(&coreParameters->cExtraPixelsLeft,
        &coreParameters->cExtraPixelsTop, &coreParameters->cExtraPixelsRight,
        &coreParameters->cExtraPixelsBottom, &orientation);
    imageInfo->cWidth = geometry.imageWidth;
    imageInfo->cHeight = geometry.imageHeight;
    parameters->cLeftX = geometry.expandedLeft;
    parameters->cTopY = geometry.expandedTop;
    parameters->cWidth = geometry.expandedWidth;
    parameters->cHeight = geometry.expandedHeight;

    streamParameters->cNumOfSliceMinus1V = (U32)(tileLayout.columnCount - 1);
    streamParameters->cNumOfSliceMinus1H = (U32)(tileLayout.rowCount - 1);
    for (boundaryIndex = 0; boundaryIndex < tileLayout.columnCount; boundaryIndex++)
        streamParameters->uiTileX[boundaryIndex] = tileLayout.columnBoundaries[boundaryIndex];
    for (boundaryIndex = 0; boundaryIndex < tileLayout.rowCount; boundaryIndex++)
        streamParameters->uiTileY[boundaryIndex] = tileLayout.rowBoundaries[boundaryIndex];
    return ICERR_OK;
}

Int JxrTranscodeRoiInitializerInitialize(CWMImageStrCodec* decoderCodec,
    CWMImageStrCodec* encoderCodec, CWMTranscodingParam* parameters,
    const JxrTranscodeOrientationState* orientation,
    JxrTranscodeRoiInitializationResult* result)
{
    size_t macroblockLeft;
    size_t macroblockRight;
    size_t macroblockTop;
    size_t macroblockBottom;

    if (decoderCodec == NULL || encoderCodec == NULL || parameters == NULL ||
        orientation == NULL || result == NULL) return ICERR_ERROR;
    if (JxrTranscodeRoiInitializerApply(&encoderCodec->WMII,
        &encoderCodec->m_param, &encoderCodec->WMISCP, parameters) != ICERR_OK)
        return ICERR_ERROR;

    macroblockLeft = parameters->cLeftX >> 4;
    macroblockRight = (parameters->cLeftX + parameters->cWidth + 15) >> 4;
    macroblockTop = parameters->cTopY >> 4;
    macroblockBottom = (parameters->cTopY + parameters->cHeight + 15) >> 4;
    if (decoderCodec->WMISCP.uiTileX[decoderCodec->WMISCP.cNumOfSliceMinus1V] >= macroblockLeft &&
        decoderCodec->WMISCP.uiTileX[decoderCodec->WMISCP.cNumOfSliceMinus1V] <= macroblockRight &&
        decoderCodec->WMISCP.uiTileY[decoderCodec->WMISCP.cNumOfSliceMinus1H] >= macroblockTop &&
        decoderCodec->WMISCP.uiTileY[decoderCodec->WMISCP.cNumOfSliceMinus1H] <= macroblockBottom)
        parameters->bIgnoreOverlap = FALSE;

    encoderCodec->bTileExtraction = parameters->bIgnoreOverlap;
    encoderCodec->cmbWidth = macroblockRight - macroblockLeft;
    encoderCodec->cmbHeight = macroblockBottom - macroblockTop;
    if (orientation->transpose) {
        JxrTranscodeRoiInitializerSwapSize(&encoderCodec->WMII.cWidth,
            &encoderCodec->WMII.cHeight);
        JxrTranscodeRoiInitializerSwapSize(&encoderCodec->cmbWidth,
            &encoderCodec->cmbHeight);
    }

    result->macroblockLeft = macroblockLeft;
    result->macroblockRight = macroblockRight;
    result->macroblockTop = macroblockTop;
    result->macroblockBottom = macroblockBottom;
    result->macroblockWidth = macroblockRight - macroblockLeft;
    result->macroblockHeight = macroblockBottom - macroblockTop;
    return ICERR_OK;
}
