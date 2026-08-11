#include "JxrVariableLengthWordReader.h"

static Bool JxrVariableLengthWordReaderReadLegacyBits(Void* context, U32 count, U32* value)
{
    *value = getBit32((BitIOInfo*)context, count);
    return TRUE;
}

Void JxrVariableLengthWordReaderInitLegacy(JxrDecoderBitSource* source,
    BitIOInfo* legacyInput)
{
    JxrDecoderBitSourceInit(source, legacyInput, JxrVariableLengthWordReaderReadLegacyBits);
}

Bool JxrVariableLengthWordReaderRead(JxrDecoderBitSource* source, U64* value,
    U8* escapeMarker)
{
    U32 bits;
    U32 firstByte;
    U64 result;

    if (source == NULL || source->read == NULL || value == NULL) return FALSE;
    if (escapeMarker != NULL) *escapeMarker = 0;
    if (!source->read(source->context, 8, &bits)) return FALSE;
    if (bits >= 0xfd) {
        if (escapeMarker != NULL) *escapeMarker = (U8)bits;
        *value = 0;
        return TRUE;
    }
    firstByte = bits;
    if (firstByte < 0xfb) {
        if (!source->read(source->context, 8, &bits)) return FALSE;
        *value = ((U64)firstByte << 8) | bits;
        return TRUE;
    }

    result = 0;
    if (bits - 0xfb != 0) {
        if (!source->read(source->context, 16, &bits)) return FALSE;
        result = (U64)bits << 16;
        if (!source->read(source->context, 16, &bits)) return FALSE;
        result = (result | bits) << 16;
        result <<= 16;
    }
    if (!source->read(source->context, 16, &bits)) return FALSE;
    result |= (U64)bits << 16;
    if (!source->read(source->context, 16, &bits)) return FALSE;
    *value = result | bits;
    return TRUE;
}
