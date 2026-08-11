#ifndef JXR_DECODER_PACKET_ROW_READER_H
#define JXR_DECODER_PACKET_ROW_READER_H

#include "JxrDecoderPacketAttachment.h"
#include "JxrDecoderPacketHeaderReader.h"
#include "JxrDecoderCodingContextResetter.h"

typedef struct JxrDecoderPacketRowReaderOperations {
    JxrDecoderPacketAttachmentOperations attachment;
    JxrDecoderPacketHeaderReaderOperations header;
    JxrDecoderCodingContextResetterOperations resetter;
} JxrDecoderPacketRowReaderOperations;

/* Reads, attaches and resets one primary decoder packet row. */
Bool JxrDecoderPacketRowReaderRead(CWMImageStrCodec* codec,
    const JxrDecoderPacketRowReaderOperations* operations);

#endif
