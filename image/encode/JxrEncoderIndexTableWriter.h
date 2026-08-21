#ifndef JXR_ENCODER_INDEX_TABLE_WRITER_H
#define JXR_ENCODER_INDEX_TABLE_WRITER_H

#include "strcodec.h"

enum { JXR_ENCODER_MINIMUM_PACKET_LENGTH = 4 };

/* Immutable index-table layout for one encoded image plane. */
typedef struct JxrEncoderIndexTablePlan {
    size_t packetGroupCount;
    size_t entryCount;
} JxrEncoderIndexTablePlan;

Void JxrEncoderIndexTablePlanInitialize(
    JxrEncoderIndexTablePlan* plan,
    size_t bitStreamCount,
    U32 horizontalSliceCountMinusOne,
    BITSTREAMFORMAT bitstreamFormat,
    Bool progressiveMode,
    U8 subbandPacketCount);

Int JxrEncoderIndexTableWriterWriteNull(CWMImageStrCodec* codec);
Int JxrEncoderIndexTableWriterWrite(CWMImageStrCodec* codec);

#endif
