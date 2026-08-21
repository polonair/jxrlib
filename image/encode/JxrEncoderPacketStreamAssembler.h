#ifndef JXR_ENCODER_PACKET_STREAM_ASSEMBLER_H
#define JXR_ENCODER_PACKET_STREAM_ASSEMBLER_H

#include "strcodec.h"

/* Immutable output ordering for encoder packet streams. */
typedef struct JxrEncoderPacketStreamPlan {
    Bool usesSpatialLayout;
    Bool usesProgressiveFrequencyLayout;
    size_t packetGroupCount;
    size_t horizontalTileCount;
    size_t verticalTileCount;
    U8 subbandPacketCount;
} JxrEncoderPacketStreamPlan;

Void JxrEncoderPacketStreamPlanInitialize(
    JxrEncoderPacketStreamPlan* plan,
    BITSTREAMFORMAT bitstreamFormat,
    Bool progressiveMode,
    U8 subbandPacketCount,
    U32 horizontalSliceCountMinusOne,
    U32 verticalSliceCountMinusOne);

/* Compatibility entry point shared with the transcode path. */
Int copyTo(struct WMPStream* source, struct WMPStream* destination, size_t byteCount);

Int JxrEncoderPacketStreamAssemblerAssemble(CWMImageStrCodec* codec);

#endif
