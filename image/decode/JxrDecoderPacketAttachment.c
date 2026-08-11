#include "JxrDecoderPacketAttachment.h"

Void JxrDecoderPacketAttachmentConfigInit(JxrDecoderPacketAttachmentConfig* config,
    const JxrDecoderBitstreamSet* bitstreams, U32 tileRow, U32 tileRowCount,
    Bool usesExternalStreams, BitIOInfo* headerReader, BitIOInfo** readers,
    const size_t* indexTable, U64 headerSize, struct WMPStream* primaryStream,
    struct WMPStream** externalStreams, U32 externalStreamCount)
{
    config->bitstreams = bitstreams;
    config->tileRow = tileRow;
    config->tileRowCount = tileRowCount;
    config->usesExternalStreams = usesExternalStreams;
    config->headerReader = headerReader;
    config->readers = readers;
    config->indexTable = indexTable;
    config->headerSize = headerSize;
    config->primaryStream = primaryStream;
    config->externalStreams = externalStreams;
    config->externalStreamCount = externalStreamCount;
}

static Bool JxrDecoderPacketAttachmentAttachReader(
    const JxrDecoderPacketAttachmentConfig* config,
    const JxrDecoderPacketAttachmentOperations* operations, BitIOInfo* reader,
    U32 readerIndex)
{
    struct WMPStream* stream;
    U32 streamIndex;

    if (config->tileRow > 0 && reader->pWS != NULL &&
        !operations->detach(operations->context, reader)) return FALSE;
    if (config->usesExternalStreams) {
        streamIndex = config->tileRow * config->bitstreams->bitstreamCount + readerIndex;
        if (streamIndex >= config->externalStreamCount) return FALSE;
        stream = config->externalStreams[streamIndex];
        return stream == NULL || operations->attach(operations->context, reader, stream);
    }
    if (!operations->seek(operations->context, config->primaryStream,
        (U64)config->indexTable[config->tileRow * config->bitstreams->bitstreamCount + readerIndex] +
        config->headerSize)) return FALSE;
    return operations->attach(operations->context, reader, config->primaryStream);
}

Bool JxrDecoderPacketAttachmentAttachRow(const JxrDecoderPacketAttachmentConfig* config,
    const JxrDecoderPacketAttachmentOperations* operations)
{
    U32 index;

    if (config == NULL || operations == NULL || config->bitstreams == NULL ||
        operations->detach == NULL || operations->attach == NULL || operations->seek == NULL ||
        config->tileRow >= config->tileRowCount) return FALSE;
    if (config->bitstreams->usesHeaderStream) {
        if (config->headerReader == NULL || !operations->detach(operations->context, config->headerReader))
            return FALSE;
        if (config->usesExternalStreams) {
            if (config->externalStreams == NULL || config->externalStreamCount == 0 ||
                config->externalStreams[0] == NULL) return FALSE;
            return operations->attach(operations->context, config->headerReader, config->externalStreams[0]);
        }
        if (config->primaryStream == NULL || !operations->seek(operations->context,
            config->primaryStream, config->headerSize)) return FALSE;
        return operations->attach(operations->context, config->headerReader, config->primaryStream);
    }
    if (config->readers == NULL) return FALSE;
    if (config->usesExternalStreams) {
        if (config->externalStreams == NULL) return FALSE;
    }
    else if (config->primaryStream == NULL || config->indexTable == NULL) return FALSE;

    for (index = 0; index < config->bitstreams->bitstreamCount; ++index) {
        if (config->readers[index] == NULL || !JxrDecoderPacketAttachmentAttachReader(
            config, operations, config->readers[index], index)) return FALSE;
    }
    return TRUE;
}
