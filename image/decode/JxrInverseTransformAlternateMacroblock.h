#ifndef JXR_INVERSE_TRANSFORM_ALTERNATE_MACROBLOCK_H
#define JXR_INVERSE_TRANSFORM_ALTERNATE_MACROBLOCK_H

#include "JxrInverseTransformBoundaryContext.h"
#include "JxrInverseTransformPlanePlan.h"
#include "JxrInverseTransformPlaneContext.h"

typedef struct JxrInverseTransformAlternateMacroblock {
    const JxrInverseTransformMacroblockGeometry* geometry;
    const JxrInverseTransformPlanePlan* planePlan;
    const JxrInverseTransformBoundaryContext* boundaries;
    const JxrInversePostProcessParameters* postProcessParameters;
    PixelI* const* firstStagePlanes;
    PixelI* const* secondStagePlanes;
    struct tagPostProcInfo* postProcessInfo[MAX_CHANNELS][2];
    PixelI (*predictionBefore)[2];
    PixelI (*predictionAfter)[2];
    size_t macroblockColumn;
    Bool usesScaledArithmetic;
} JxrInverseTransformAlternateMacroblock;

Void JxrInverseTransformAlternateMacroblockProcess(
    JxrInverseTransformAlternateMacroblock* macroblock);

#endif
