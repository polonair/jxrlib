#ifndef JXR_DECODER_FORMAT_STATE_H
#define JXR_DECODER_FORMAT_STATE_H

#include "strcodec.h"

/* Read-only decoder format and tile configuration. */
typedef struct JxrDecoderFormatState {
    CWMImageStrCodec* nativeCodec;
} JxrDecoderFormatState;

Void JxrDecoderFormatStateInit(JxrDecoderFormatState* state, CWMImageStrCodec* nativeCodec);
COLORFORMAT JxrDecoderFormatStateGetColorFormat(const JxrDecoderFormatState* state);
Int JxrDecoderFormatStateGetChannelCount(const JxrDecoderFormatState* state);
Bool JxrDecoderFormatStateIsSpatial(const JxrDecoderFormatState* state);
Bool JxrDecoderFormatStateIsDcOnly(const JxrDecoderFormatState* state);
Bool JxrDecoderFormatStateHasHighpass(const JxrDecoderFormatState* state);
CWMITile* JxrDecoderFormatStateGetCurrentTile(const JxrDecoderFormatState* state);

#endif
