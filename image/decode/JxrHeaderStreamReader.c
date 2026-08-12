#include "JxrHeaderStreamReader.h"
#include <string.h>

Bool JxrHeaderStreamReaderOpen(JxrHeaderStreamReader* reader,
    struct WMPStream* stream)
{
    U8 signature[8];
    if (reader == NULL || stream == NULL || stream->Read == NULL) return FALSE;
    memset(reader, 0, sizeof(*reader));
    if (stream->Read(stream, signature, sizeof(signature)) != WMP_errSuccess ||
        memcmp(signature, gGDISignature, sizeof(signature)) != 0 ||
        attach_SB(&reader->bitInput, stream) != WMP_errSuccess) return FALSE;
    reader->stream = stream;
    reader->attached = TRUE;
    return TRUE;
}

SimpleBitIO* JxrHeaderStreamReaderGetBitInput(JxrHeaderStreamReader* reader)
{
    return reader != NULL && reader->attached ? &reader->bitInput : NULL;
}

Bool JxrHeaderStreamReaderAlignToByte(JxrHeaderStreamReader* reader)
{
    if (reader == NULL || !reader->attached) return FALSE;
    flushToByte_SB(&reader->bitInput);
    return TRUE;
}

Bool JxrHeaderStreamReaderClose(JxrHeaderStreamReader* reader, U32* bytesRead)
{
    if (reader == NULL || bytesRead == NULL || !reader->attached ||
        !JxrHeaderStreamReaderAlignToByte(reader)) return FALSE;
    *bytesRead = getByteRead_SB(&reader->bitInput);
    if (detach_SB(&reader->bitInput) != WMP_errSuccess) return FALSE;
    reader->stream = NULL;
    reader->attached = FALSE;
    return TRUE;
}
