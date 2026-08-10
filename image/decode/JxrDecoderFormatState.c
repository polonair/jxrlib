#include "JxrDecoderFormatState.h"
#include "decode.h"
#include "JxrSubbandStreamRefill.h"

#include <assert.h>

Void JxrDecoderTileStateInit(JxrDecoderTileState* state, const CWMITile* nativeTile)
{
    Int channel;
    state->lowpassQuantizerBits = 0;
    state->highpassQuantizerBits = 0;
    state->lowpassQuantizerCount = 0;
    state->highpassQuantizerCount = 0;
    for (channel = 0; channel < MAX_CHANNELS; ++channel)
        state->highpassQuantizers[channel] = NULL;
    if (nativeTile == NULL) return;
    state->lowpassQuantizerBits = nativeTile->cBitsLP;
    state->highpassQuantizerBits = nativeTile->cBitsHP;
    state->lowpassQuantizerCount = nativeTile->cNumQPLP;
    state->highpassQuantizerCount = nativeTile->cNumQPHP;
    for (channel = 0; channel < MAX_CHANNELS; ++channel)
        state->highpassQuantizers[channel] = nativeTile->pQuantizerHP[channel];
}

U8 JxrDecoderTileStateGetLowpassQuantizerBits(const JxrDecoderTileState* state)
{ return state->lowpassQuantizerBits; }
U8 JxrDecoderTileStateGetHighpassQuantizerBits(const JxrDecoderTileState* state)
{ return state->highpassQuantizerBits; }
U8 JxrDecoderTileStateGetLowpassQuantizerCount(const JxrDecoderTileState* state)
{ return state->lowpassQuantizerCount; }
U8 JxrDecoderTileStateGetHighpassQuantizerCount(const JxrDecoderTileState* state)
{ return state->highpassQuantizerCount; }
Int JxrDecoderTileStateGetHighpassQuantizerParameter(const JxrDecoderTileState* state,
    Int plane, Int quantizerIndex)
{
    assert(plane >= 0 && plane < MAX_CHANNELS);
    assert(quantizerIndex >= 0 && quantizerIndex < state->highpassQuantizerCount);
    assert(state->highpassQuantizers[plane] != NULL);
    return state->highpassQuantizers[plane][quantizerIndex].iQP;
}

Void JxrDecoderFormatStateInit(JxrDecoderFormatState* state, CWMImageStrCodec* nativeCodec)
{
    state->nativeCodec = nativeCodec;
    state->colorFormat = Y_ONLY;
    state->channelCount = 0;
    state->isSpatial = FALSE;
    state->isDcOnly = FALSE;
    state->hasHighpass = FALSE;
    JxrDecoderTileStateInit(&state->currentTile, NULL);
    state->shouldResetScan = FALSE;
    state->shouldResetContext = FALSE;
    state->isTranscode = FALSE;
    state->hasFlexbits = FALSE;
    state->shouldSkipFlexbits = FALSE;
    state->shouldAdaptDcHuffman = FALSE;
    if (nativeCodec == NULL) return;

    state->colorFormat = nativeCodec->m_param.cfColorFormat;
    state->channelCount = (Int)nativeCodec->m_param.cNumChannels;
    state->isSpatial = nativeCodec->WMISCP.bfBitstreamFormat == SPATIAL;
    state->isDcOnly = nativeCodec->WMISCP.sbSubband == SB_DC_ONLY;
    state->hasHighpass = nativeCodec->WMISCP.sbSubband != SB_NO_HIGHPASS;
    if (nativeCodec->pTile != NULL)
        JxrDecoderTileStateInit(&state->currentTile,
            nativeCodec->pTile + nativeCodec->cTileColumn);
    state->shouldResetScan = nativeCodec->m_bResetRGITotals;
    state->shouldResetContext = nativeCodec->m_bResetContext;
    state->isTranscode = nativeCodec->m_param.bTranscode;
    state->hasFlexbits = nativeCodec->WMISCP.sbSubband != SB_NO_FLEXBITS;
    if (nativeCodec->m_Dparam != NULL) {
        state->shouldSkipFlexbits = nativeCodec->m_Dparam->bSkipFlexbits;
        state->shouldAdaptDcHuffman =
            nativeCodec->WMISCP.bfBitstreamFormat == FREQUENCY &&
            nativeCodec->m_Dparam->cThumbnailScale >= 16;
    }
    state->shouldAdaptDcHuffman = state->shouldAdaptDcHuffman || state->isDcOnly;
}
COLORFORMAT JxrDecoderFormatStateGetColorFormat(const JxrDecoderFormatState* state)
{ return state->colorFormat; }
Int JxrDecoderFormatStateGetChannelCount(const JxrDecoderFormatState* state)
{ return state->channelCount; }
Bool JxrDecoderFormatStateIsSpatial(const JxrDecoderFormatState* state)
{ return state->isSpatial; }
Bool JxrDecoderFormatStateIsDcOnly(const JxrDecoderFormatState* state)
{ return state->isDcOnly; }
Bool JxrDecoderFormatStateHasHighpass(const JxrDecoderFormatState* state)
{ return state->hasHighpass; }
const JxrDecoderTileState* JxrDecoderFormatStateGetCurrentTile(const JxrDecoderFormatState* state)
{ return &state->currentTile; }
Bool JxrDecoderFormatStateShouldResetScan(const JxrDecoderFormatState* state)
{ return state->shouldResetScan; }
Bool JxrDecoderFormatStateShouldResetContext(const JxrDecoderFormatState* state)
{ return state->shouldResetContext; }
Bool JxrDecoderFormatStateIsTranscode(const JxrDecoderFormatState* state)
{ return state->isTranscode; }
Bool JxrDecoderFormatStateHasFlexbits(const JxrDecoderFormatState* state)
{ return state->hasFlexbits; }
Bool JxrDecoderFormatStateShouldSkipFlexbits(const JxrDecoderFormatState* state)
{ return state->shouldSkipFlexbits; }
Bool JxrDecoderFormatStateShouldAdaptDcHuffman(const JxrDecoderFormatState* state)
{ return state->shouldAdaptDcHuffman; }
Bool JxrDecoderFormatStateRefillLevel1(JxrDecoderFormatState* state, JxrEntropyBitReader* reader)
{ return JxrSubbandStreamRefillLevel1(state->nativeCodec, reader); }
Bool JxrDecoderFormatStateRefillLevel2(JxrDecoderFormatState* state, JxrEntropyBitReader* reader)
{ return JxrSubbandStreamRefillLevel2(state->nativeCodec, reader); }
