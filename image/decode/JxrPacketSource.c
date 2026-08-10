#include "JxrPacketSource.h"
JxrPacketReadResult JxrPacketSourceRead(JxrPacketSource* source, size_t offset,
    U8* destination, size_t count)
{
    JxrPacketReadResult result;
    result.status = JxrPacketReadFailed;
    result.bytesRead = 0;
    result.nativeError = WMP_errFileIO;
    if (source != NULL && source->readAt != NULL)
        result = source->readAt(source->context, offset, destination, count);
    return result;
}
