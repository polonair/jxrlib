#include "JxrDecoderPacketHeaderReader.h"

Void JxrDecoderPacketHeaderReaderConfigInit(JxrDecoderPacketHeaderReaderConfig* config,
    const JxrDecoderBitstreamSet* bitstreams, U32 tileRow, Bool trimFlexbits,
    BitIOInfo* headerReader, BitIOInfo** readers)
{
    config->bitstreams = bitstreams;
    config->tileRow = tileRow;
    config->trimFlexbits = trimFlexbits;
    config->headerReader = headerReader;
    config->readers = readers;
}

static Bool JxrDecoderPacketHeaderReaderReadRequired(
    const JxrDecoderPacketHeaderReaderOperations* operations, BitIOInfo* reader,
    U8 packetType, U8 packetId)
{
    return reader != NULL && reader->pWS != NULL &&
        operations->readHeader(operations->context, reader, packetType, packetId);
}

Bool JxrDecoderPacketHeaderReaderReadRow(const JxrDecoderPacketHeaderReaderConfig* config,
    const JxrDecoderPacketHeaderReaderOperations* operations)
{
    U32 tileColumn;

    if (config == NULL || operations == NULL || config->bitstreams == NULL ||
        operations->readHeader == NULL || operations->storeTrim == NULL ||
        (config->trimFlexbits && operations->readTrim == NULL) ||
        (!config->bitstreams->isSpatial && config->readers == NULL)) return FALSE;
    for (tileColumn = 0; tileColumn < config->bitstreams->tileColumnCount; ++tileColumn) {
        U8 packetId = (U8)((config->tileRow * config->bitstreams->tileColumnCount + tileColumn) & 0x1F);
        Int trimFlexBits = 0;
        BitIOInfo* reader;

        if (config->bitstreams->usesHeaderStream) reader = config->headerReader;
        else if (config->readers != NULL) reader = config->readers[tileColumn * config->bitstreams->bitstreamsPerTile];
        else return FALSE;
        if (config->bitstreams->isSpatial) {
            U32 trimValue;
            if (!JxrDecoderPacketHeaderReaderReadRequired(operations, reader, 0, packetId)) return FALSE;
            if (config->trimFlexbits) {
                if (!operations->readTrim(operations->context, reader, &trimValue)) return FALSE;
                trimFlexBits = (Int)trimValue;
            }
        }
        else {
            U32 base = tileColumn * config->bitstreams->bitstreamsPerTile;
            U32 trimValue;

            if (!JxrDecoderPacketHeaderReaderReadRequired(operations, config->readers[base], 1, packetId))
                return FALSE;
            if (config->bitstreams->bitstreamsPerTile > 1 &&
                !JxrDecoderPacketHeaderReaderReadRequired(operations, config->readers[base + 1], 2, packetId))
                return FALSE;
            if (config->bitstreams->bitstreamsPerTile > 2 &&
                !JxrDecoderPacketHeaderReaderReadRequired(operations, config->readers[base + 2], 3, packetId))
                return FALSE;
            if (config->bitstreams->bitstreamsPerTile > 3) {
                reader = config->readers[base + 3];
                if (reader == NULL || reader->pWS == NULL) return FALSE;
                operations->readHeader(operations->context, reader, 4, packetId);
                if (config->trimFlexbits) {
                    if (!operations->readTrim(operations->context, reader, &trimValue)) return FALSE;
                    trimFlexBits = (Int)trimValue;
                }
            }
        }
        if ((config->bitstreams->isSpatial || config->bitstreams->bitstreamsPerTile > 3) &&
            !operations->storeTrim(operations->context, tileColumn, trimFlexBits)) return FALSE;
    }
    return TRUE;
}
