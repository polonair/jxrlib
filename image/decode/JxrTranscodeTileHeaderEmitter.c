#include "JxrTranscodeTileHeaderEmitter.h"

#include "JxrTranscodeTileHeaderWriter.h"

Void JxrTranscodeTileHeaderEmitterEmit(CWMImageStrCodec* codec,
    JxrTranscodeTileQuantizerState* quantizers)
{
    CCodingContext* context;
    CWMITile* tile;
    CWMImageStrCodec* alphaCodec;
    JxrTranscodeBitSink dcOutput;
    JxrTranscodeBitSink lowpassOutput;
    JxrTranscodeBitSink highpassOutput;
    JxrTranscodeBitSink flexbitsOutput;
    JxrTranscodeTileHeaderState state = {0};
    JxrTranscodeTileHeaderResult result;

    if (codec == NULL || quantizers == NULL || !codec->m_bCtxLeft ||
        !codec->m_bCtxTop || codec->m_bSecondary) return;
    context = &codec->m_pCodingContext[codec->cTileColumn];
    tile = codec->pTile + codec->cTileColumn;
    alphaCodec = codec->m_param.bAlphaChannel ? codec->m_pNextSC : NULL;
    JxrTranscodeBitSinkInitLegacy(&dcOutput, context->m_pIODC);
    JxrTranscodeBitSinkInitLegacy(&lowpassOutput, context->m_pIOLP);
    JxrTranscodeBitSinkInitLegacy(&highpassOutput, context->m_pIOAC);
    JxrTranscodeBitSinkInitLegacy(&flexbitsOutput, context->m_pIOFL);
    state.isSpatial = codec->WMISCP.bfBitstreamFormat == SPATIAL;
    state.subband = codec->WMISCP.sbSubband;
    state.quantizerMode = codec->m_param.uQPMode;
    state.hasAlpha = alphaCodec != NULL;
    state.trimFlexbits = codec->m_param.bTrimFlexbitsFlag;
    state.trimFlexbitsValue = (U8)context->m_iTrimFlexBits;
    state.tileId = (U8)((codec->cTileRow * (codec->WMISCP.cNumOfSliceMinus1V + 1) +
        codec->cTileColumn) & 0x1F);
    state.channelCount = codec->WMISCP.cChannel;
    state.alphaChannelIndex = codec->m_param.cNumChannels;
    state.quantizers = quantizers;
    state.dcOutput = &dcOutput;
    state.lowpassOutput = &lowpassOutput;
    state.highpassOutput = &highpassOutput;
    state.flexbitsOutput = &flexbitsOutput;
    if (JxrTranscodeTileHeaderWriterWrite(&state, &result) == FALSE) return;
    tile->cBitsLP = result.lowpassQuantizerBits;
    tile->cBitsHP = result.highpassQuantizerBits;
    if (alphaCodec != NULL) {
        tile = alphaCodec->pTile + codec->cTileColumn;
        tile->cBitsLP = result.lowpassAlphaQuantizerBits;
        tile->cBitsHP = result.highpassAlphaQuantizerBits;
    }
}
