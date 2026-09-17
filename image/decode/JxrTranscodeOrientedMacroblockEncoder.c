#include "JxrTranscodeOrientedMacroblockEncoder.h"

#include "../encode/encode.h"

static Bool JxrTranscodeOrientedMacroblockEncoderIsValid(
    const JxrTranscodeOrientedMacroblockEncoderRequest* request)
{
    return request != NULL && request->destinationCodec != NULL &&
        request->primaryMacroblocks != NULL && request->primaryCoefficients != NULL &&
        request->destinationCoefficients != NULL && request->orientation != NULL &&
        request->tileQuantizers != NULL && request->tileQuantizerCount > 0 &&
        request->tileColumnCount > 0 &&
        (!request->hasAlpha || (request->sourceAlphaCodec != NULL &&
            request->destinationAlphaCodec != NULL &&
            request->alphaMacroblocks != NULL && request->alphaCoefficients != NULL &&
            request->alphaDestinationCoefficients != NULL));
}

Int JxrTranscodeOrientedMacroblockEncoderEncode(
    const JxrTranscodeOrientedMacroblockEncoderRequest* request)
{
    CWMImageStrCodec* destinationCodec;

    if (!JxrTranscodeOrientedMacroblockEncoderIsValid(request)) return ICERR_ERROR;
    destinationCodec = request->destinationCodec;
    for (destinationCodec->cRow = 1; destinationCodec->cRow <= destinationCodec->cmbHeight;
        ++destinationCodec->cRow) {
        for (destinationCodec->cColumn = 1; destinationCodec->cColumn <= destinationCodec->cmbWidth;
            ++destinationCodec->cColumn) {
            Int macroblockRow = (Int)destinationCodec->cRow - 1;
            Int macroblockColumn = (Int)destinationCodec->cColumn - 1;
            size_t macroblockOffset = (destinationCodec->cRow - 1) * destinationCodec->cmbWidth +
                destinationCodec->cColumn - 1;
            size_t tileQuantizerOffset;
            JxrTranscodeMacroblockTransformState primaryTransform = {0};

            if (macroblockOffset >= request->macroblockCount)
                return ICERR_ERROR;
            primaryTransform.sourceMacroblocks = request->primaryMacroblocks;
            primaryTransform.sourceCoefficients = request->primaryCoefficients;
            primaryTransform.coefficientUnit = request->coefficientUnit;
            primaryTransform.macroblockOffset = macroblockOffset;
            primaryTransform.destinationCodec = destinationCodec;
            primaryTransform.destinationCoefficients = request->destinationCoefficients;
            primaryTransform.orientation = request->orientation;
            if (!JxrTranscodeMacroblockTransformPrimary(&primaryTransform)) return ICERR_ERROR;
            getTilePos(destinationCodec, macroblockColumn, macroblockRow);
            if (destinationCodec->cTileRow > ((size_t)-1 - destinationCodec->cTileColumn) /
                request->tileColumnCount) return ICERR_ERROR;
            tileQuantizerOffset = destinationCodec->cTileRow * request->tileColumnCount +
                destinationCodec->cTileColumn;
            if (tileQuantizerOffset >= request->tileQuantizerCount) return ICERR_ERROR;
            if (destinationCodec->m_bCtxLeft && destinationCodec->m_bCtxTop)
                JxrTranscodeTileHeaderEmitterEmit(destinationCodec,
                    request->tileQuantizers + tileQuantizerOffset);
            if (encodeMB(destinationCodec, macroblockColumn, macroblockRow) != ICERR_OK)
                return ICERR_ERROR;
            if (request->hasAlpha) {
                CWMImageStrCodec* alphaCodec = request->destinationAlphaCodec;
                JxrTranscodeMacroblockTransformState alphaTransform = {0};

                alphaCodec->cColumn = destinationCodec->cColumn;
                alphaCodec->cRow = destinationCodec->cRow;
                getTilePos(alphaCodec, macroblockColumn, macroblockRow);
                alphaCodec->MBInfo = request->sourceAlphaCodec->MBInfo;
                alphaTransform.sourceMacroblocks = request->alphaMacroblocks;
                alphaTransform.sourceCoefficients = request->alphaCoefficients;
                alphaTransform.coefficientUnit = 256;
                alphaTransform.macroblockOffset = macroblockOffset;
                alphaTransform.destinationCodec = alphaCodec;
                alphaTransform.destinationCoefficients = request->alphaDestinationCoefficients;
                alphaTransform.orientation = request->orientation;
                if (!JxrTranscodeMacroblockTransformAlpha(&alphaTransform) ||
                    encodeMB(alphaCodec, macroblockColumn, macroblockRow) != ICERR_OK)
                    return ICERR_ERROR;
            }
        }
        advanceOneMBRow(destinationCodec);
    }
    return ICERR_OK;
}
