#include "JxrPacketSource.h"
Bool JxrPacketSourceRead(JxrPacketSource* source, size_t offset, U8* destination, size_t count)
{ return source != NULL && source->readAt != NULL && source->readAt(source->context, offset, destination, count); }
