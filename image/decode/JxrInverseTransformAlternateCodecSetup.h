#ifndef JXR_INVERSE_TRANSFORM_ALTERNATE_CODEC_SETUP_H
#define JXR_INVERSE_TRANSFORM_ALTERNATE_CODEC_SETUP_H

#include "JxrHardTileCodecStateAdapter.h"
#include "JxrInversePostProcessParameters.h"
#include "JxrInverseTransformPlanePlan.h"

typedef struct JxrInverseTransformAlternateCodecSetup {
    JxrInverseTransformMacroblockGeometry geometry;
    JxrInverseTransformPlanePlan planePlan;
    JxrHardTileBoundaryState hardTileState;
    JxrInverseTransformBoundaryContext boundaryContext;
    JxrInversePostProcessParameters postProcessParameters;
} JxrInverseTransformAlternateCodecSetup;

Void JxrInverseTransformAlternateCodecSetupInitialize(
    JxrInverseTransformAlternateCodecSetup* setup,
    CWMImageStrCodec* codec);

#endif
