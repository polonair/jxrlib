#ifndef JXR_ENCODER_TILE_STATE_INITIALIZER_H
#define JXR_ENCODER_TILE_STATE_INITIALIZER_H

#include "strcodec.h"

typedef struct JxrEncoderTileStatePlan {
    Bool supportsTileCount;
    Int codingContextCount;
} JxrEncoderTileStatePlan;

Void JxrEncoderTileStatePlanInitialize(JxrEncoderTileStatePlan* plan,
    U32 verticalSlicesMinusOne);
Int JxrEncoderTileStateInitializerInitialize(CWMImageStrCodec* codec);

#endif
