#include "JxrDecoderFormatState.h"
#include "decode.h"
#include "JxrSubbandStreamRefill.h"

Void JxrDecoderFormatStateInit(JxrDecoderFormatState* state, CWMImageStrCodec* nativeCodec)
{
    state->nativeCodec = nativeCodec;
    state->colorFormat = Y_ONLY;
    state->channelCount = 0;
    state->isSpatial = FALSE;
    state->isDcOnly = FALSE;
    state->hasHighpass = FALSE;
    state->currentTile = NULL;
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
        state->currentTile = nativeCodec->pTile + nativeCodec->cTileColumn;
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
CWMITile* JxrDecoderFormatStateGetCurrentTile(const JxrDecoderFormatState* state)
{ return state->currentTile; }
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
