#ifndef JXR_DECODER_TILE_QUANTIZER_SYNTAX_READER_H
#define JXR_DECODER_TILE_QUANTIZER_SYNTAX_READER_H

#include "strcodec.h"

#define JXR_DECODER_TILE_MAX_QUANTIZERS 16

typedef Bool (*JxrDecoderBitSourceRead)(Void* context, U32 count, U32* value);
typedef struct JxrDecoderBitSource { Void* context; JxrDecoderBitSourceRead read; } JxrDecoderBitSource;

typedef struct JxrDecoderQuantizerSyntax {
    U8 channelMode;
    U8 indices[MAX_CHANNELS];
} JxrDecoderQuantizerSyntax;

typedef struct JxrDecoderQuantizerSetSyntax {
    Bool copyPrevious;
    U8 count;
    JxrDecoderQuantizerSyntax values[JXR_DECODER_TILE_MAX_QUANTIZERS];
} JxrDecoderQuantizerSetSyntax;

Void JxrDecoderBitSourceInit(JxrDecoderBitSource* source, Void* context,
    JxrDecoderBitSourceRead read);
Void JxrDecoderBitSourceInitLegacy(JxrDecoderBitSource* source, BitIOInfo* legacyInput);
Bool JxrDecoderTileQuantizerSyntaxReadDc(JxrDecoderBitSource* source, size_t channelCount,
    JxrDecoderQuantizerSyntax* result);
Bool JxrDecoderTileQuantizerSyntaxReadLowpass(JxrDecoderBitSource* source,
    size_t channelCount, JxrDecoderQuantizerSetSyntax* result);
Bool JxrDecoderTileQuantizerSyntaxReadHighpass(JxrDecoderBitSource* source,
    size_t channelCount, U8 lowpassCount, JxrDecoderQuantizerSetSyntax* result);

#endif
