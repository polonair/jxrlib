#ifndef JXR_DECODER_PACKET_ATTACHMENT_H
#define JXR_DECODER_PACKET_ATTACHMENT_H

#include "JxrDecoderBitstreamSet.h"

typedef Bool (*JxrDecoderPacketDetach)(Void* context, BitIOInfo* reader);
typedef Bool (*JxrDecoderPacketAttach)(Void* context, BitIOInfo* reader, struct WMPStream* stream);
typedef Bool (*JxrDecoderPacketSeek)(Void* context, struct WMPStream* stream, U64 offset);

typedef struct JxrDecoderPacketAttachmentOperations {
    Void* context;
    JxrDecoderPacketDetach detach;
    JxrDecoderPacketAttach attach;
    JxrDecoderPacketSeek seek;
} JxrDecoderPacketAttachmentOperations;

typedef struct JxrDecoderPacketAttachmentConfig {
    const JxrDecoderBitstreamSet* bitstreams;
    U32 tileRow;
    U32 tileRowCount;
    Bool usesExternalStreams;
    BitIOInfo* headerReader;
    BitIOInfo** readers;
    const size_t* indexTable;
    U64 headerSize;
    struct WMPStream* primaryStream;
    struct WMPStream** externalStreams;
    U32 externalStreamCount;
} JxrDecoderPacketAttachmentConfig;

Void JxrDecoderPacketAttachmentConfigInit(JxrDecoderPacketAttachmentConfig* config,
    const JxrDecoderBitstreamSet* bitstreams, U32 tileRow, U32 tileRowCount,
    Bool usesExternalStreams, BitIOInfo* headerReader, BitIOInfo** readers,
    const size_t* indexTable, U64 headerSize, struct WMPStream* primaryStream,
    struct WMPStream** externalStreams, U32 externalStreamCount);
Bool JxrDecoderPacketAttachmentAttachRow(const JxrDecoderPacketAttachmentConfig* config,
    const JxrDecoderPacketAttachmentOperations* operations);

#endif
