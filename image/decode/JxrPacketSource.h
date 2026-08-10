#ifndef JXR_PACKET_SOURCE_H
#define JXR_PACKET_SOURCE_H

#include "windowsmediaphoto.h"

typedef Bool (*JxrPacketSourceReadAt)(Void* context, size_t offset, U8* destination, size_t count);
typedef struct JxrPacketSource { Void* context; JxrPacketSourceReadAt readAt; } JxrPacketSource;
Bool JxrPacketSourceRead(JxrPacketSource* source, size_t offset, U8* destination, size_t count);

#endif
