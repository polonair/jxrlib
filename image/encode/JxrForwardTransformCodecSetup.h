#ifndef JXR_FORWARD_TRANSFORM_CODEC_SETUP_H
#define JXR_FORWARD_TRANSFORM_CODEC_SETUP_H

#include "JxrForwardTransformBoundaryContext.h"

/* Per-macroblock forward transform state derived from the codec. */
typedef struct JxrForwardTransformCodecSetup {
    JxrForwardTransformMacroblockGeometry geometry;
    JxrForwardHardTileBoundaryState hardTileState;
    JxrForwardTransformBoundaryContext boundaries;
    Bool usesScaledArithmetic;
} JxrForwardTransformCodecSetup;

Void JxrForwardTransformCodecSetupInitialize(
    JxrForwardTransformCodecSetup* setup,
    CWMImageStrCodec* codec);

#endif
