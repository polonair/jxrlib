#ifndef JXR_INVERSE_TRANSFORM_NORMAL_CODEC_SETUP_H
#define JXR_INVERSE_TRANSFORM_NORMAL_CODEC_SETUP_H

#include "JxrInverseHighPassParameters.h"
#include "JxrInversePostProcessParameters.h"
#include "JxrInverseTransformMacroblockGeometry.h"
#include "JxrInverseTransformPlanePlan.h"

typedef struct JxrInverseTransformNormalCodecSetup {
    JxrInverseTransformMacroblockGeometry geometry;
    JxrInverseTransformPlanePlan planePlan;
    JxrInversePostProcessParameters postProcessParameters;
    JxrInverseHighPassParameters highPassParameters;
} JxrInverseTransformNormalCodecSetup;

Void JxrInverseTransformNormalCodecSetupInitialize(
    JxrInverseTransformNormalCodecSetup* setup,
    const CWMImageStrCodec* codec);

#endif
