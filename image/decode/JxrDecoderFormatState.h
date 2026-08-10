#ifndef JXR_DECODER_FORMAT_STATE_H
#define JXR_DECODER_FORMAT_STATE_H

#include "strcodec.h"
#include "JxrEntropyReader.h"

/* Read-only decoder configuration snapshot plus temporary native transport access. */
typedef struct JxrDecoderFormatState {
    /* Retained only for packet refill until WMPStream is ported. */
    CWMImageStrCodec* nativeCodec;
    COLORFORMAT colorFormat;
    Int channelCount;
    Bool isSpatial;
    Bool isDcOnly;
    Bool hasHighpass;
    CWMITile* currentTile;
    Bool shouldResetScan;
    Bool shouldResetContext;
    Bool isTranscode;
    Bool hasFlexbits;
    Bool shouldSkipFlexbits;
    Bool shouldAdaptDcHuffman;
} JxrDecoderFormatState;

Void JxrDecoderFormatStateInit(JxrDecoderFormatState* state, CWMImageStrCodec* nativeCodec);
COLORFORMAT JxrDecoderFormatStateGetColorFormat(const JxrDecoderFormatState* state);
Int JxrDecoderFormatStateGetChannelCount(const JxrDecoderFormatState* state);
Bool JxrDecoderFormatStateIsSpatial(const JxrDecoderFormatState* state);
Bool JxrDecoderFormatStateIsDcOnly(const JxrDecoderFormatState* state);
Bool JxrDecoderFormatStateHasHighpass(const JxrDecoderFormatState* state);
CWMITile* JxrDecoderFormatStateGetCurrentTile(const JxrDecoderFormatState* state);
Bool JxrDecoderFormatStateShouldResetScan(const JxrDecoderFormatState* state);
Bool JxrDecoderFormatStateShouldResetContext(const JxrDecoderFormatState* state);
Bool JxrDecoderFormatStateIsTranscode(const JxrDecoderFormatState* state);
Bool JxrDecoderFormatStateHasFlexbits(const JxrDecoderFormatState* state);
Bool JxrDecoderFormatStateShouldSkipFlexbits(const JxrDecoderFormatState* state);
Bool JxrDecoderFormatStateShouldAdaptDcHuffman(const JxrDecoderFormatState* state);
Bool JxrDecoderFormatStateRefillLevel1(JxrDecoderFormatState* state, JxrEntropyBitReader* reader);
Bool JxrDecoderFormatStateRefillLevel2(JxrDecoderFormatState* state, JxrEntropyBitReader* reader);

#endif
