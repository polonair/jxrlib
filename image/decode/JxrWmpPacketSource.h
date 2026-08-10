#ifndef JXR_WMP_PACKET_SOURCE_H
#define JXR_WMP_PACKET_SOURCE_H

#include "JxrPacketSource.h"
#include "strcodec.h"

typedef struct JxrWmpPacketSource {
    struct WMPStream* stream;
} JxrWmpPacketSource;

Void JxrWmpPacketSourceInit(JxrWmpPacketSource* state, struct WMPStream* stream,
    JxrPacketSource* source);

#endif
