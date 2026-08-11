#include "JxrDecoderLpQuantizerHeaderApplier.h"

Bool JxrDecoderLpQuantizerHeaderApplierApply(CWMImageStrCodec* codec,
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
    tile->bUseDC = syntax->copyPrevious;
    tile->cNumQPLP = syntax->count;
    tile->cBitsLP = tile->bUseDC ? 0 : dquantBits(tile->cNumQPLP);
    if (codec->cTileRow > 0) freeQuantizer(tile->pQuantizerLP);
    if (allocateQuantizer(tile->pQuantizerLP, codec->m_param.cNumChannels,
        tile->cNumQPLP) != ICERR_OK)
        return FALSE;

    if (tile->bUseDC) {
        useDCQuantizer(codec, codec->cTileColumn);
        return TRUE;
    }

    for (quantizer = 0; quantizer < tile->cNumQPLP; ++quantizer) {
        tile->cChModeLP[quantizer] = syntax->values[quantizer].channelMode;
        for (channel = 0; channel < codec->m_param.cNumChannels; ++channel)
            tile->pQuantizerLP[channel][quantizer].iIndex =
                syntax->values[quantizer].indices[channel];
        formatQuantizer(tile->pQuantizerLP, tile->cChModeLP[quantizer],
            codec->m_param.cNumChannels, quantizer, TRUE, codec->m_param.bScaledArith);
    }
    return TRUE;
}
