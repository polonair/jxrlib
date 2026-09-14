#include "JxrTranscodeDirectMacroblockEncoder.h"

#include "../encode/encode.h"
#include "JxrTranscodeTileHeaderEmitter.h"

Int JxrTranscodeDirectMacroblockEncoderEncode(CWMImageStrCodec* sourceCodec,
    CWMImageStrCodec* destinationCodec, size_t macroblockLeft, size_t macroblockTop,
    Int destinationColumn, Int destinationRow,
    JxrTranscodeTileQuantizerState* quantizers, Bool hasAlpha)
{
    if (sourceCodec == NULL || destinationCodec == NULL || quantizers == NULL ||
        (hasAlpha && (sourceCodec->m_pNextSC == NULL ||
            destinationCodec->m_pNextSC == NULL))) return ICERR_ERROR;
    destinationCodec->cColumn = sourceCodec->cColumn - macroblockLeft + 1;
    destinationCodec->cRow = sourceCodec->cRow - macroblockTop + 1;
    destinationCodec->MBInfo = sourceCodec->MBInfo;
    getTilePos(destinationCodec, destinationColumn, destinationRow);
    if (destinationCodec->m_bCtxLeft && destinationCodec->m_bCtxTop)
        JxrTranscodeTileHeaderEmitterEmit(destinationCodec, quantizers);
    if (encodeMB(destinationCodec, destinationColumn, destinationRow) != ICERR_OK)
        return ICERR_ERROR;
    if (hasAlpha) {
        destinationCodec->m_pNextSC->cColumn = destinationCodec->cColumn;
        destinationCodec->m_pNextSC->cRow = destinationCodec->cRow;
        getTilePos(destinationCodec->m_pNextSC, destinationColumn, destinationRow);
        destinationCodec->m_pNextSC->MBInfo = sourceCodec->m_pNextSC->MBInfo;
        if (encodeMB(destinationCodec->m_pNextSC, destinationColumn,
            destinationRow) != ICERR_OK) return ICERR_ERROR;
    }
    return ICERR_OK;
}
