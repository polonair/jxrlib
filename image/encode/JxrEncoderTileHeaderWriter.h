#ifndef JXR_ENCODER_TILE_HEADER_WRITER_H
#define JXR_ENCODER_TILE_HEADER_WRITER_H

#include "strcodec.h"

/* Immutable quantizer-header selection for one encoder tile. */
typedef struct JxrEncoderTileHeaderPlan {
    Bool writesDcQuantizer;
    Bool writesLpQuantizer;
    Bool writesHpQuantizer;
} JxrEncoderTileHeaderPlan;

Void JxrEncoderTileHeaderPlanInitialize(
    JxrEncoderTileHeaderPlan* plan,
    U32 quantizerMode,
    SUBBAND subband);

Void JxrEncoderTileHeaderWriterWriteQuantizer(
    CWMIQuantizer* quantizers[MAX_CHANNELS],
    BitIOInfo* bitWriter,
    U8 channelMode,
    size_t channelCount,
    size_t quantizerIndex);

Int JxrEncoderTileHeaderWriterWriteDc(
    CWMImageStrCodec* codec,
    BitIOInfo* bitWriter);
Int JxrEncoderTileHeaderWriterWriteLp(
    CWMImageStrCodec* codec,
    BitIOInfo* bitWriter);
Int JxrEncoderTileHeaderWriterWriteHp(
    CWMImageStrCodec* codec,
    BitIOInfo* bitWriter);

#endif
