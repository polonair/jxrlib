#ifndef JXR_HARD_TILE_CODEC_STATE_ADAPTER_H
#define JXR_HARD_TILE_CODEC_STATE_ADAPTER_H

#include "JxrInverseTransformBoundaryContext.h"

/* Legacy codec bridge: reads, calculates, and stores hard-tile state. */
Void JxrHardTileCodecStateAdapterUpdate(
    CWMImageStrCodec* codec,
    const JxrInverseTransformMacroblockGeometry* geometry,
    JxrHardTileBoundaryState* hardTileState,
    JxrInverseTransformBoundaryContext* boundaryContext);

#endif
