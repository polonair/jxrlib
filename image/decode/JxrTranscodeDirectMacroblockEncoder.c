#include "JxrTranscodeDirectMacroblockEncoder.h"

#include "../encode/encode.h"
#include "JxrTranscodeTileHeaderEmitter.h"

Int JxrTranscodeDirectMacroblockEncoderEncode(
    const JxrTranscodePlanePair* sourcePlanes,
    const JxrTranscodePlanePair* destinationPlanes, size_t macroblockLeft, size_t macroblockTop,
    Int destinationColumn, Int destinationRow,
    JxrTranscodeTileQuantizerState* quantizers)
{
    CWMImageStrCodec* sourceCodec;
    CWMImageStrCodec* destinationCodec;
    Bool hasAlpha;

    if (sourcePlanes == NULL || destinationPlanes == NULL) return ICERR_ERROR;
    sourceCodec = sourcePlanes->primaryCodec;
    destinationCodec = destinationPlanes->primaryCodec;
    hasAlpha = sourcePlanes->hasAlpha;
    if (sourceCodec == NULL || destinationCodec == NULL || quantizers == NULL ||
        hasAlpha != destinationPlanes->hasAlpha ||
        (hasAlpha && (sourcePlanes->alphaCodec == NULL ||
            destinationPlanes->alphaCodec == NULL))) return ICERR_ERROR;
    destinationCodec->cColumn = sourceCodec->cColumn - macroblockLeft + 1;
    destinationCodec->cRow = sourceCodec->cRow - macroblockTop + 1;
    destinationCodec->MBInfo = sourceCodec->MBInfo;
    getTilePos(destinationCodec, destinationColumn, destinationRow);
    if (destinationCodec->m_bCtxLeft && destinationCodec->m_bCtxTop)
        JxrTranscodeTileHeaderEmitterEmit(destinationCodec, quantizers);
    if (encodeMB(destinationCodec, destinationColumn, destinationRow) != ICERR_OK)
        return ICERR_ERROR;
    if (hasAlpha) {
        destinationPlanes->alphaCodec->cColumn = destinationCodec->cColumn;
        destinationPlanes->alphaCodec->cRow = destinationCodec->cRow;
        getTilePos(destinationPlanes->alphaCodec, destinationColumn, destinationRow);
        destinationPlanes->alphaCodec->MBInfo = sourcePlanes->alphaCodec->MBInfo;
        if (encodeMB(destinationPlanes->alphaCodec, destinationColumn,
            destinationRow) != ICERR_OK) return ICERR_ERROR;
    }
    return ICERR_OK;
}
