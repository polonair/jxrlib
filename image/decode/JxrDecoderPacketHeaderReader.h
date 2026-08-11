#ifndef JXR_DECODER_PACKET_HEADER_READER_H
#define JXR_DECODER_PACKET_HEADER_READER_H

#include "JxrDecoderBitstreamSet.h"

typedef Bool (*JxrDecoderPacketHeaderRead)(Void* context, BitIOInfo* reader,
    U8 packetType, U8 packetId);
typedef Bool (*JxrDecoderPacketTrimRead)(Void* context, BitIOInfo* reader, U32* value);
typedef Bool (*JxrDecoderPacketTrimStore)(Void* context, U32 tileColumn, Int value);

typedef struct JxrDecoderPacketHeaderReaderOperations {
    Void* context;
    JxrDecoderPacketHeaderRead readHeader;
    JxrDecoderPacketTrimRead readTrim;
    JxrDecoderPacketTrimStore storeTrim;
} JxrDecoderPacketHeaderReaderOperations;

typedef struct JxrDecoderPacketHeaderReaderConfig {
    const JxrDecoderBitstreamSet* bitstreams;
    U32 tileRow;
    Bool trimFlexbits;
    BitIOInfo* headerReader;
    BitIOInfo** readers;
} JxrDecoderPacketHeaderReaderConfig;

Void JxrDecoderPacketHeaderReaderConfigInit(JxrDecoderPacketHeaderReaderConfig* config,
    const JxrDecoderBitstreamSet* bitstreams, U32 tileRow, Bool trimFlexbits,
    BitIOInfo* headerReader, BitIOInfo** readers);
Bool JxrDecoderPacketHeaderReaderReadRow(const JxrDecoderPacketHeaderReaderConfig* config,
    const JxrDecoderPacketHeaderReaderOperations* operations);

#endif
