#include "JxrPacketHeaderSyntaxReader.h"

Bool JxrPacketHeaderSyntaxReaderRead(JxrDecoderBitSource* source,
    JxrPacketHeaderSyntax* header)
{
    U32 value;

    if (source == NULL || source->read == NULL || header == NULL) return FALSE;
    if (!source->read(source->context, 8, &value)) return FALSE;
    header->prefix0 = (U8)value;
    if (!source->read(source->context, 8, &value)) return FALSE;
    header->prefix1 = (U8)value;
    if (!source->read(source->context, 8, &value)) return FALSE;
    header->marker = (U8)value;
    if (!source->read(source->context, 8, &value)) return FALSE;
    header->tileAndType = (U8)value;
    return TRUE;
}

Bool JxrPacketHeaderSyntaxIsValid(const JxrPacketHeaderSyntax* header)
{
    return header != NULL && header->prefix0 == 0 && header->prefix1 == 0 &&
        header->marker == 1;
}

U8 JxrPacketHeaderSyntaxGetTileId(const JxrPacketHeaderSyntax* header)
{
    return header == NULL ? 0 : (U8)(header->tileAndType >> 3);
}

U8 JxrPacketHeaderSyntaxGetPacketType(const JxrPacketHeaderSyntax* header)
{
    return header == NULL ? 0 : (U8)(header->tileAndType & 7);
}
