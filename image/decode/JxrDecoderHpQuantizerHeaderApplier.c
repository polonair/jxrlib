#include "JxrDecoderHpQuantizerHeaderApplier.h"

Bool JxrDecoderHpQuantizerHeaderApplierApply(CWMImageStrCodec* codec,
    const JxrDecoderQuantizerSetSyntax* syntax)
{
    CWMITile* tile;
    U8 quantizer;
    size_t channel;

    if (codec == NULL || syntax == NULL || codec->pTile == NULL ||
        codec->m_param.cNumChannels == 0 || codec->m_param.cNumChannels > MAX_CHANNELS ||
        syntax->count == 0 || syntax->count > JXR_DECODER_TILE_MAX_QUANTIZERS)
        return FALSE;

    tile = codec->pTile + codec->cTileColumn;
    tile->bUseLP = syntax->copyPrevious;
    tile->cNumQPHP = syntax->count;
    tile->cBitsHP = tile->bUseLP ? 0 : dquantBits(tile->cNumQPHP);
    if (codec->cTileRow > 0) freeQuantizer(tile->pQuantizerHP);
    if (allocateQuantizer(tile->pQuantizerHP, codec->m_param.cNumChannels,
        tile->cNumQPHP) != ICERR_OK)
        return FALSE;

    if (tile->bUseLP) {
        useLPQuantizer(codec, tile->cNumQPHP, codec->cTileColumn);
        return TRUE;
    }

    for (quantizer = 0; quantizer < tile->cNumQPHP; ++quantizer) {
        tile->cChModeHP[quantizer] = syntax->values[quantizer].channelMode;
        for (channel = 0; channel < codec->m_param.cNumChannels; ++channel)
            tile->pQuantizerHP[channel][quantizer].iIndex =
                syntax->values[quantizer].indices[channel];
        formatQuantizer(tile->pQuantizerHP, tile->cChModeHP[quantizer],
            codec->m_param.cNumChannels, quantizer, FALSE, codec->m_param.bScaledArith);
    }
    return TRUE;
}
