#include "JxrDecoderDcQuantizerHeaderApplier.h"

Bool JxrDecoderDcQuantizerHeaderApplierApply(CWMImageStrCodec* codec,
    const JxrDecoderQuantizerSyntax* syntax)
{
    CWMITile* tile;
    size_t channel;
    size_t tileIndex;

    if (codec == NULL || syntax == NULL || codec->pTile == NULL ||
        codec->m_param.cNumChannels == 0 || codec->m_param.cNumChannels > MAX_CHANNELS)
        return FALSE;

    if (codec->cTileRow + codec->cTileColumn == 0)
        for (tileIndex = 0; tileIndex <= codec->WMISCP.cNumOfSliceMinus1V; ++tileIndex)
            if (allocateQuantizer(codec->pTile[tileIndex].pQuantizerDC,
                codec->m_param.cNumChannels, 1) != ICERR_OK)
                return FALSE;

    tile = codec->pTile + codec->cTileColumn;
    tile->cChModeDC = syntax->channelMode;
    for (channel = 0; channel < codec->m_param.cNumChannels; ++channel)
        tile->pQuantizerDC[channel][0].iIndex = syntax->indices[channel];
    formatQuantizer(tile->pQuantizerDC, tile->cChModeDC, codec->m_param.cNumChannels,
        0, TRUE, codec->m_param.bScaledArith);
    return TRUE;
}
