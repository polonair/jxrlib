#include "JxrDecoderFormatState.h"

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
