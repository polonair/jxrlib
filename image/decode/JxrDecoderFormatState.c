#include "JxrDecoderFormatState.h"
#include "decode.h"
#include "JxrSubbandStreamRefill.h"

Void JxrDecoderFormatStateInit(JxrDecoderFormatState* state, CWMImageStrCodec* nativeCodec)
{ state->nativeCodec = nativeCodec; }
COLORFORMAT JxrDecoderFormatStateGetColorFormat(const JxrDecoderFormatState* state)
{ return state->nativeCodec->m_param.cfColorFormat; }
Int JxrDecoderFormatStateGetChannelCount(const JxrDecoderFormatState* state)
{ return (Int)state->nativeCodec->m_param.cNumChannels; }
Bool JxrDecoderFormatStateIsSpatial(const JxrDecoderFormatState* state)
{ return state->nativeCodec->WMISCP.bfBitstreamFormat == SPATIAL; }
Bool JxrDecoderFormatStateIsDcOnly(const JxrDecoderFormatState* state)
{ return state->nativeCodec->WMISCP.sbSubband == SB_DC_ONLY; }
Bool JxrDecoderFormatStateHasHighpass(const JxrDecoderFormatState* state)
{ return state->nativeCodec->WMISCP.sbSubband != SB_NO_HIGHPASS; }
CWMITile* JxrDecoderFormatStateGetCurrentTile(const JxrDecoderFormatState* state)
{ return state->nativeCodec->pTile + state->nativeCodec->cTileColumn; }
Bool JxrDecoderFormatStateShouldResetScan(const JxrDecoderFormatState* state)
{ return state->nativeCodec->m_bResetRGITotals; }
Bool JxrDecoderFormatStateShouldResetContext(const JxrDecoderFormatState* state)
{ return state->nativeCodec->m_bResetContext; }
Bool JxrDecoderFormatStateIsTranscode(const JxrDecoderFormatState* state)
{ return state->nativeCodec->m_param.bTranscode; }
Bool JxrDecoderFormatStateHasFlexbits(const JxrDecoderFormatState* state)
{ return state->nativeCodec->WMISCP.sbSubband != SB_NO_FLEXBITS; }
Bool JxrDecoderFormatStateShouldSkipFlexbits(const JxrDecoderFormatState* state)
{ return state->nativeCodec->m_Dparam->bSkipFlexbits; }
Bool JxrDecoderFormatStateShouldAdaptDcHuffman(const JxrDecoderFormatState* state)
{ return (!(state->nativeCodec->WMISCP.bfBitstreamFormat != FREQUENCY || state->nativeCodec->m_Dparam->cThumbnailScale < 16)) || JxrDecoderFormatStateIsDcOnly(state); }
Bool JxrDecoderFormatStateRefillLevel1(JxrDecoderFormatState* state, JxrEntropyBitReader* reader)
{ return JxrSubbandStreamRefillLevel1(state->nativeCodec, reader); }
Bool JxrDecoderFormatStateRefillLevel2(JxrDecoderFormatState* state, JxrEntropyBitReader* reader)
{ return JxrSubbandStreamRefillLevel2(state->nativeCodec, reader); }
