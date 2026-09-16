#include "JxrTranscodeEncoderOutputInitializer.h"

EXTERN_C Int StrEncInit(CWMImageStrCodec*);
EXTERN_C Int WriteWMIHeader(CWMImageStrCodec*);

static Void JxrTranscodeEncoderOutputInitializerComposeOrientation(
    CWMImageStrCodec* encoderCodec, ORIENTATION orientationValue,
    const JxrTranscodeOrientationState* orientation)
{
    JxrTranscodeOrientationState sourceOrientation;

    JxrTranscodeOrientationStateInit(&sourceOrientation,
        encoderCodec->WMII.oOrientation);
    if (!orientation->transpose && !sourceOrientation.transpose)
        encoderCodec->WMII.oOrientation ^= orientationValue;
    else if (orientation->transpose && sourceOrientation.transpose) {
        encoderCodec->WMII.oOrientation ^= orientationValue;
        encoderCodec->WMII.oOrientation = (encoderCodec->WMII.oOrientation & 1) * 2 +
            (encoderCodec->WMII.oOrientation >> 1);
    }
    else if (orientation->transpose)
        encoderCodec->WMII.oOrientation = orientationValue ^
            ((encoderCodec->WMII.oOrientation & 1) * 2 +
            (encoderCodec->WMII.oOrientation >> 1));
    else
        encoderCodec->WMII.oOrientation ^= ((orientationValue & 1) * 2 +
            (orientationValue >> 1));
}

static Int JxrTranscodeEncoderOutputInitializerAllocateTileQuantizers(
    CWMImageStrCodec* encoderCodec, ORIENTATION orientationValue,
    JxrTranscodeEncoderOutputInitializationResult* result)
{
    size_t rowCount;
    size_t columnCount;
    size_t tileCount;

    if (orientationValue == O_NONE)
        tileCount = 1;
    else {
        rowCount = (size_t)encoderCodec->WMISCP.cNumOfSliceMinus1H + 1;
        columnCount = (size_t)encoderCodec->WMISCP.cNumOfSliceMinus1V + 1;
        if (columnCount != 0 && rowCount > ((size_t)-1) / columnCount)
            return ICERR_ERROR;
        tileCount = rowCount * columnCount;
    }
    if (tileCount > ((size_t)-1) / sizeof(JxrTranscodeTileQuantizerState))
        return ICERR_ERROR;
    result->tileQuantizers = (JxrTranscodeTileQuantizerState*)malloc(tileCount *
        sizeof(JxrTranscodeTileQuantizerState));
    if (result->tileQuantizers == NULL) return ICERR_ERROR;
    result->tileQuantizerCount = tileCount;
    return ICERR_OK;
}

Void JxrTranscodeEncoderOutputInitializerRelease(
    JxrTranscodeEncoderOutputInitializationResult* result)
{
    if (result == NULL) return;
    free(result->tileQuantizers);
    memset(result, 0, sizeof(*result));
}

Int JxrTranscodeEncoderOutputInitializerInitialize(CWMImageStrCodec* decoderCodec,
    CWMImageStrCodec* encoderCodec, CWMTranscodingParam* parameters,
    ORIENTATION orientationValue, const JxrTranscodeOrientationState* orientation,
    JxrTranscodeEncoderOutputInitializationResult* result)
{
    if (decoderCodec == NULL || encoderCodec == NULL || parameters == NULL ||
        orientation == NULL || result == NULL)
        return ICERR_ERROR;
    memset(result, 0, sizeof(*result));
    JxrTranscodeEncoderOutputInitializerComposeOrientation(encoderCodec,
        orientationValue, orientation);
    if (parameters->bIgnoreOverlap) {
        attachISWrite(encoderCodec->pIOHeader, encoderCodec->WMISCP.pWStream);
        encoderCodec->pTile = decoderCodec->pTile;
        if (encoderCodec->WMISCP.cNumOfSliceMinus1H +
            encoderCodec->WMISCP.cNumOfSliceMinus1V == 0 &&
            encoderCodec->WMISCP.bfBitstreamFormat == SPATIAL)
            encoderCodec->m_param.bIndexTable = FALSE;
        WriteWMIHeader(encoderCodec);
        result->usedFastTileExtraction = TRUE;
        return ICERR_OK;
    }
    if (JxrTranscodeEncoderOutputInitializerAllocateTileQuantizers(encoderCodec,
        orientationValue, result) != ICERR_OK)
        return ICERR_ERROR;
    if (StrEncInit(encoderCodec) != ICERR_OK) {
        JxrTranscodeEncoderOutputInitializerRelease(result);
        return ICERR_ERROR;
    }
    return ICERR_OK;
}
