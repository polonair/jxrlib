#include "JxrIndexTableReader.h"

static Bool JxrIndexTableReaderReadLegacyBits(Void* context, U32 count, U32* value)
{
    JxrIndexTableLegacyContext* legacyContext = (JxrIndexTableLegacyContext*)context;
    *value = getBit32(legacyContext->input, count);
    return TRUE;
}

static Bool JxrIndexTableReaderRefillLegacy(Void* context)
{
    JxrIndexTableLegacyContext* legacyContext = (JxrIndexTableLegacyContext*)context;
    readIS_L1(legacyContext->codec, legacyContext->input);
    return TRUE;
}

static Bool JxrIndexTableReaderAlignLegacy(Void* context)
{
    JxrIndexTableLegacyContext* legacyContext = (JxrIndexTableLegacyContext*)context;
    flushToByte(legacyContext->input);
    return TRUE;
}

static U64 JxrIndexTableReaderGetLegacyPosition(Void* context)
{
    JxrIndexTableLegacyContext* legacyContext = (JxrIndexTableLegacyContext*)context;
    return (U64)getPosRead(legacyContext->input);
}

Void JxrIndexTableReaderInit(JxrIndexTableReader* reader, Void* context,
    JxrDecoderBitSourceRead readBits, JxrIndexTableOperation refill,
    JxrIndexTableOperation alignToByte, JxrIndexTablePosition getPosition)
{
    JxrDecoderBitSourceInit(&reader->bitSource, context, readBits);
    reader->context = context;
    reader->refill = refill;
    reader->alignToByte = alignToByte;
    reader->getPosition = getPosition;
}

Void JxrIndexTableReaderInitLegacy(JxrIndexTableReader* reader,
    JxrIndexTableLegacyContext* legacyContext)
{
    JxrIndexTableReaderInit(reader, legacyContext, JxrIndexTableReaderReadLegacyBits,
        JxrIndexTableReaderRefillLegacy, JxrIndexTableReaderAlignLegacy,
        JxrIndexTableReaderGetLegacyPosition);
}

Bool JxrIndexTableReaderRead(JxrIndexTableReader* reader, U32 entryCount,
    JxrIndexTableEntryConsumer consumeEntry, Void* entryContext, U64* headerSize)
{
    U32 value;
    U32 index;
    U64 variableLengthWord;

    if (reader == NULL || headerSize == NULL || reader->bitSource.read == NULL ||
        reader->refill == NULL || reader->alignToByte == NULL || reader->getPosition == NULL ||
        (entryCount != 0 && consumeEntry == NULL)) return FALSE;
    if (!reader->refill(reader->context)) return FALSE;
    if (entryCount != 0) {
        if (!reader->bitSource.read(reader->bitSource.context, 16, &value) || value != 1)
            return FALSE;
        for (index = 0; index < entryCount; ++index) {
            if (!reader->refill(reader->context) ||
                !JxrVariableLengthWordReaderRead(&reader->bitSource, &variableLengthWord, NULL))
                return FALSE;
            if (!consumeEntry(entryContext, index, variableLengthWord)) return FALSE;
        }
    }
    if (!JxrVariableLengthWordReaderRead(&reader->bitSource, &variableLengthWord, NULL) ||
        !reader->alignToByte(reader->context)) return FALSE;
    *headerSize = variableLengthWord + reader->getPosition(reader->context);
    return TRUE;
}
