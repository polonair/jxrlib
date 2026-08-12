#ifndef JXR_HEADER_STREAM_READER_H
#define JXR_HEADER_STREAM_READER_H

#include "JxrHeaderValidation.h"

typedef struct JxrHeaderStreamReader {
    SimpleBitIO bitInput;
    struct WMPStream* stream;
    Bool attached;
} JxrHeaderStreamReader;

Bool JxrHeaderStreamReaderOpen(JxrHeaderStreamReader* reader,
    struct WMPStream* stream);
SimpleBitIO* JxrHeaderStreamReaderGetBitInput(JxrHeaderStreamReader* reader);
Bool JxrHeaderStreamReaderAlignToByte(JxrHeaderStreamReader* reader);
Bool JxrHeaderStreamReaderClose(JxrHeaderStreamReader* reader, U32* bytesRead);

#endif
