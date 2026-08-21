#ifndef JXR_ENCODER_PACKET_HEADER_WRITER_H
#define JXR_ENCODER_PACKET_HEADER_WRITER_H

#include "strcodec.h"

/* Immutable packet-header selection for the current tile. */
typedef struct JxrEncoderPacketHeaderPlan {
    Bool writesHeaders;
    Bool usesSpatialLayout;
    Bool writesLowpassHeader;
    Bool writesHighpassHeader;
    Bool writesFlexbitsPacket;
    Bool writesTrimFlexbits;
} JxrEncoderPacketHeaderPlan;

Void JxrEncoderPacketHeaderPlanInitialize(
    JxrEncoderPacketHeaderPlan* plan,
    BITSTREAMFORMAT bitstreamFormat,
    U8 subbandPacketCount,
    Bool trimFlexbits,
    Bool contextLeft,
    Bool contextTop,
    Bool isSecondary,
    Bool isTranscode);

/* Writes all packet and tile headers required for one tile's first macroblock. */
Void JxrEncoderPacketHeaderWriterWrite(
    CWMImageStrCodec* codec,
    CCodingContext* codingContext);

#endif
