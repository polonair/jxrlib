#include "JxrDecoderTileQuantizerSyntaxReader.h"

static Bool JxrDecoderBitSourceReadLegacy(Void* context, U32 count, U32* value)
{
    *value = getBit16((BitIOInfo*)context, count);
    return TRUE;
}

static Bool JxrDecoderBitSourceReadSimple(Void* context, U32 count, U32* value)
{
    *value = getBit32_SB((SimpleBitIO*)context, count);
    return TRUE;
}

static Bool JxrDecoderTileQuantizerSyntaxReadQuantizer(JxrDecoderBitSource* source,
    size_t channelCount, JxrDecoderQuantizerSyntax* result)
{
    U32 value;
    size_t channel;

    if (source == NULL || source->read == NULL || result == NULL || channelCount == 0 ||
        channelCount > MAX_CHANNELS) return FALSE;
    result->channelMode = 0;
    if (channelCount > 1) {
        if (!source->read(source->context, 2, &value)) return FALSE;
        result->channelMode = (U8)value;
    }
    if (!source->read(source->context, 8, &value)) return FALSE;
    result->indices[0] = (U8)value;
    if (result->channelMode == 1) {
        if (!source->read(source->context, 8, &value)) return FALSE;
        result->indices[1] = (U8)value;
    }
    else if (result->channelMode > 0)
        for (channel = 1; channel < channelCount; ++channel) {
            if (!source->read(source->context, 8, &value)) return FALSE;
            result->indices[channel] = (U8)value;
        }
    return TRUE;
}

static Bool JxrDecoderTileQuantizerSyntaxReadSet(JxrDecoderBitSource* source,
    size_t channelCount, U8 copyCount, JxrDecoderQuantizerSetSyntax* result)
{
    U32 value;
    U8 index;

    if (source == NULL || result == NULL || copyCount == 0) return FALSE;
    if (!source->read(source->context, 1, &value)) return FALSE;
    result->copyPrevious = value == 1;
    result->count = copyCount;
    if (result->copyPrevious) return TRUE;
    if (!source->read(source->context, 4, &value)) return FALSE;
    result->count = (U8)value + 1;
    for (index = 0; index < result->count; ++index)
        if (!JxrDecoderTileQuantizerSyntaxReadQuantizer(source, channelCount,
            &result->values[index])) return FALSE;
    return TRUE;
}

Void JxrDecoderBitSourceInit(JxrDecoderBitSource* source, Void* context,
    JxrDecoderBitSourceRead read)
{ source->context = context; source->read = read; }
Void JxrDecoderBitSourceInitLegacy(JxrDecoderBitSource* source, BitIOInfo* legacyInput)
{ JxrDecoderBitSourceInit(source, legacyInput, JxrDecoderBitSourceReadLegacy); }
Void JxrDecoderBitSourceInitSimple(JxrDecoderBitSource* source, SimpleBitIO* simpleInput)
{ JxrDecoderBitSourceInit(source, simpleInput, JxrDecoderBitSourceReadSimple); }
Bool JxrDecoderTileQuantizerSyntaxReadDc(JxrDecoderBitSource* source, size_t channelCount,
    JxrDecoderQuantizerSyntax* result)
{ return JxrDecoderTileQuantizerSyntaxReadQuantizer(source, channelCount, result); }
Bool JxrDecoderTileQuantizerSyntaxReadLowpass(JxrDecoderBitSource* source,
    size_t channelCount, JxrDecoderQuantizerSetSyntax* result)
{ return JxrDecoderTileQuantizerSyntaxReadSet(source, channelCount, 1, result); }
Bool JxrDecoderTileQuantizerSyntaxReadHighpass(JxrDecoderBitSource* source,
    size_t channelCount, U8 lowpassCount, JxrDecoderQuantizerSetSyntax* result)
{ return JxrDecoderTileQuantizerSyntaxReadSet(source, channelCount, lowpassCount, result); }
