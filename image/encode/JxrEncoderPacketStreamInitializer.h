#ifndef JXR_ENCODER_PACKET_STREAM_INITIALIZER_H
#define JXR_ENCODER_PACKET_STREAM_INITIALIZER_H

#include "strcodec.h"

/* Immutable storage and index decisions for encoder packet streams. */
typedef struct JxrEncoderPacketStreamInitializationPlan {
    Bool writesIndexTable;
    Bool createsPacketStreams;
    Bool usesTemporaryFiles;
} JxrEncoderPacketStreamInitializationPlan;

Void JxrEncoderPacketStreamInitializationPlanInitialize(
    JxrEncoderPacketStreamInitializationPlan* plan,
    BITSTREAMFORMAT bitstreamFormat,
    U32 verticalSliceCountMinusOne,
    U32 horizontalSliceCountMinusOne,
    size_t packetStreamCount,
    size_t macroblockWidth,
    size_t macroblockHeight,
    size_t channelCount);

Int JxrEncoderPacketStreamInitializerInitialize(CWMImageStrCodec* codec);

#endif
