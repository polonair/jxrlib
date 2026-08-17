#include "JxrInverseTransformAlternateMacroblock.h"
#include "JxrInverseTransformAlternateCodecSetup.h"
#include "JxrInverseTransformCodecInvocation.h"
#include "JxrInverseTransformAlternateFullResolutionPlane.h"
#include "JxrInverseTransformChroma420AlternatePlane.h"
#include "JxrInverseTransformChroma422AlternatePlane.h"

Void JxrInverseTransformAlternateMacroblockInitialize(
    JxrInverseTransformAlternateMacroblock* macroblock,
    const JxrInverseTransformAlternateCodecSetup* setup,
    const JxrInverseTransformCodecInvocation* invocation)
{
    macroblock->geometry = &setup->geometry;
    macroblock->planePlan = &setup->planePlan;
    macroblock->boundaries = &setup->boundaryContext;
    macroblock->postProcessParameters = &setup->postProcessParameters;
    macroblock->firstStagePlanes = invocation->firstStagePlanes;
    macroblock->secondStagePlanes = invocation->secondStagePlanes;
    memcpy(macroblock->postProcessInfo, invocation->postProcessInfo,
        sizeof(macroblock->postProcessInfo));
    macroblock->predictionBefore = invocation->predictionBefore;
    macroblock->predictionAfter = invocation->predictionAfter;
    macroblock->macroblockColumn = setup->hardTileState.previousMacroblockX;
    macroblock->usesScaledArithmetic = invocation->usesScaledArithmetic;
}

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
