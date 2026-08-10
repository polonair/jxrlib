#ifndef JXR_PACKET_SOURCE_H
#define JXR_PACKET_SOURCE_H

#include "windowsmediaphoto.h"

typedef enum JxrPacketReadStatus {
    JxrPacketReadCompleted,
    JxrPacketReadShort,
    JxrPacketReadFailed
} JxrPacketReadStatus;

typedef struct JxrPacketReadResult {
    JxrPacketReadStatus status;
    size_t bytesRead;
    Int nativeError;
} JxrPacketReadResult;

typedef JxrPacketReadResult (*JxrPacketSourceReadAt)(Void* context, size_t offset,
    U8* destination, size_t count);
typedef struct JxrPacketSource { Void* context; JxrPacketSourceReadAt readAt; } JxrPacketSource;
JxrPacketReadResult JxrPacketSourceRead(JxrPacketSource* source, size_t offset,
    U8* destination, size_t count);

#endif
