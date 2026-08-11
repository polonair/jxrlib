#ifndef JXR_PACKET_HEADER_SYNTAX_READER_H
#define JXR_PACKET_HEADER_SYNTAX_READER_H

#include "JxrDecoderTileQuantizerSyntaxReader.h"

typedef struct JxrPacketHeaderSyntax {
    U8 prefix0;
    U8 prefix1;
    U8 marker;
    U8 tileAndType;
} JxrPacketHeaderSyntax;

Bool JxrPacketHeaderSyntaxReaderRead(JxrDecoderBitSource* source,
    JxrPacketHeaderSyntax* header);
Bool JxrPacketHeaderSyntaxIsValid(const JxrPacketHeaderSyntax* header);
U8 JxrPacketHeaderSyntaxGetTileId(const JxrPacketHeaderSyntax* header);
U8 JxrPacketHeaderSyntaxGetPacketType(const JxrPacketHeaderSyntax* header);

#endif
