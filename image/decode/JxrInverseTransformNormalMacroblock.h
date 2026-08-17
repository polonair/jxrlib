#ifndef JXR_INVERSE_TRANSFORM_NORMAL_MACROBLOCK_H
#define JXR_INVERSE_TRANSFORM_NORMAL_MACROBLOCK_H

#include "JxrInverseTransformPlanePlan.h"
#include "JxrInverseTransformPlaneContext.h"
#include "JxrInverseTransformMacroblockGeometry.h"

struct JxrInverseTransformNormalCodecSetup;
struct JxrInverseTransformCodecInvocation;

typedef struct JxrInverseTransformNormalMacroblock {
    const JxrInverseTransformMacroblockGeometry* geometry;
    const JxrInverseTransformPlanePlan* planePlan;
    const JxrInversePostProcessParameters* postProcessParameters;
    const JxrInverseHighPassParameters* highPassParameters;
    PixelI* const* firstStagePlanes;
    PixelI* const* secondStagePlanes;
    struct tagPostProcInfo* postProcessInfo[MAX_CHANNELS][2];
    size_t macroblockColumn;
    Bool usesScaledArithmetic;
} JxrInverseTransformNormalMacroblock;

Void JxrInverseTransformNormalMacroblockInitialize(
    JxrInverseTransformNormalMacroblock* macroblock,
    const struct JxrInverseTransformNormalCodecSetup* setup,
    const struct JxrInverseTransformCodecInvocation* invocation);

Void JxrInverseTransformNormalMacroblockProcess(
    JxrInverseTransformNormalMacroblock* macroblock);

#endif
