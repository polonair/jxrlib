#ifndef JXR_DECODER_FORMAT_STATE_H
#define JXR_DECODER_FORMAT_STATE_H

#include "strcodec.h"
#include "JxrEntropyReader.h"

#define JXR_TILE_MAX_QUANTIZERS 16

/* Read-only tile snapshot. Native quantizer arrays remain isolated in this bridge. */
typedef struct JxrDecoderTileState {
    U8 lowpassQuantizerBits;
    U8 highpassQuantizerBits;
    U8 lowpassQuantizerCount;
    U8 highpassQuantizerCount;
    Int highpassQuantizerParameters[MAX_CHANNELS][JXR_TILE_MAX_QUANTIZERS];
} JxrDecoderTileState;

Void JxrDecoderTileStateInit(JxrDecoderTileState* state, const CWMITile* nativeTile);
U8 JxrDecoderTileStateGetLowpassQuantizerBits(const JxrDecoderTileState* state);
U8 JxrDecoderTileStateGetHighpassQuantizerBits(const JxrDecoderTileState* state);
U8 JxrDecoderTileStateGetLowpassQuantizerCount(const JxrDecoderTileState* state);
U8 JxrDecoderTileStateGetHighpassQuantizerCount(const JxrDecoderTileState* state);
Int JxrDecoderTileStateGetHighpassQuantizerParameter(const JxrDecoderTileState* state,
    Int plane, Int quantizerIndex);

/* Read-only decoder configuration snapshot plus temporary native transport access. */
typedef struct JxrDecoderFormatState {
    /* Retained only for packet refill until WMPStream is ported. */
    CWMImageStrCodec* nativeCodec;
    COLORFORMAT colorFormat;
    Int channelCount;
    Bool isSpatial;
    Bool isDcOnly;
    Bool hasHighpass;
    JxrDecoderTileState currentTile;
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
const JxrDecoderTileState* JxrDecoderFormatStateGetCurrentTile(const JxrDecoderFormatState* state);
Bool JxrDecoderFormatStateShouldResetScan(const JxrDecoderFormatState* state);
Bool JxrDecoderFormatStateShouldResetContext(const JxrDecoderFormatState* state);
Bool JxrDecoderFormatStateIsTranscode(const JxrDecoderFormatState* state);
Bool JxrDecoderFormatStateHasFlexbits(const JxrDecoderFormatState* state);
Bool JxrDecoderFormatStateShouldSkipFlexbits(const JxrDecoderFormatState* state);
Bool JxrDecoderFormatStateShouldAdaptDcHuffman(const JxrDecoderFormatState* state);
Bool JxrDecoderFormatStateRefillLevel1(JxrDecoderFormatState* state, JxrEntropyBitReader* reader);
Bool JxrDecoderFormatStateRefillLevel2(JxrDecoderFormatState* state, JxrEntropyBitReader* reader);

#endif
