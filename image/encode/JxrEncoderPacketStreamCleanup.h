#ifndef JXR_ENCODER_PACKET_STREAM_CLEANUP_H
#define JXR_ENCODER_PACKET_STREAM_CLEANUP_H

#include "strcodec.h"

enum { JXR_ENCODER_MAX_MEMORY_SIZE_IN_WORDS = 64 << 20 };

/* Immutable ownership decisions for encoder packet resources. */
typedef struct JxrEncoderPacketStreamCleanupPlan {
    Bool releasesPacketResources;
    Bool releasesTemporaryFiles;
    Bool closesPacketStreams;
} JxrEncoderPacketStreamCleanupPlan;

Void JxrEncoderPacketStreamCleanupPlanInitialize(
    JxrEncoderPacketStreamCleanupPlan* plan,
    size_t packetStreamCount,
    size_t macroblockWidth,
    size_t macroblockHeight,
    size_t channelCount);

Int JxrEncoderPacketStreamCleanupRelease(CWMImageStrCodec* codec);

#endif
