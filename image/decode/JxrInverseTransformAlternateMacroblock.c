#include "JxrInverseTransformAlternateMacroblock.h"
#include "JxrInverseTransformAlternateFullResolutionPlane.h"
#include "JxrInverseTransformChroma420AlternatePlane.h"
#include "JxrInverseTransformChroma422AlternatePlane.h"

Void JxrInverseTransformAlternateMacroblockProcess(
    JxrInverseTransformAlternateMacroblock* macroblock)
{
    size_t channel;
    JxrInverseTransformPlaneContext plane;
    const JxrInverseTransformPlanePlan* plan = macroblock->planePlan;

    for (channel = 0; channel < plan->fullResolutionChannelCount && plan->transformsSamples; ++channel) {
        JxrInverseTransformPlaneContextInitialize(&plane, macroblock->firstStagePlanes,
            macroblock->secondStagePlanes, FALSE, channel, macroblock->postProcessParameters, NULL);
        JxrInverseTransformAlternateFullResolutionPlaneApply(&plane, macroblock->geometry,
            macroblock->boundaries, macroblock->usesScaledArithmetic,
            macroblock->postProcessParameters->enabled, macroblock->postProcessInfo,
            macroblock->macroblockColumn);
    }
    for (channel = 0; channel < plan->chroma420ChannelCount && plan->transformsSamples; ++channel) {
        JxrInverseTransformPlaneContextInitialize(&plane, macroblock->firstStagePlanes,
            macroblock->secondStagePlanes, TRUE, channel, macroblock->postProcessParameters, NULL);
        JxrInverseTransformChroma420AlternatePlaneApply(&plane, macroblock->geometry,
            macroblock->boundaries, macroblock->usesScaledArithmetic,
            macroblock->predictionBefore[channel], macroblock->predictionAfter[channel]);
    }
    for (channel = 0; channel < plan->chroma422ChannelCount && plan->transformsSamples; ++channel) {
        JxrInverseTransformPlaneContextInitialize(&plane, macroblock->firstStagePlanes,
            macroblock->secondStagePlanes, TRUE, channel, macroblock->postProcessParameters, NULL);
        JxrInverseTransformChroma422AlternatePlaneApply(&plane, macroblock->geometry,
            macroblock->boundaries, macroblock->usesScaledArithmetic,
            macroblock->predictionBefore[channel], macroblock->predictionAfter[channel]);
    }
}
