#ifndef JXR_FORWARD_HARD_TILE_CODEC_STATE_ADAPTER_H
#define JXR_FORWARD_HARD_TILE_CODEC_STATE_ADAPTER_H

#include "strcodec.h"
#include "JxrForwardHardTileBoundaryState.h"

Void JxrForwardHardTileCodecStateAdapterUpdate(
    CWMImageStrCodec* codec,
    JxrForwardHardTileBoundaryState* state);

#endif
