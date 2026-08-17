#include "JxrInverseTransformNormalMacroblock.h"
#include "JxrInverseTransformCodecInvocation.h"
#include "JxrInverseTransformNormalCodecSetup.h"
#include "JxrInverseTransformFullResolutionPlane.h"
#include "JxrInverseTransformChroma420Plane.h"
#include "JxrInverseTransformChroma422Plane.h"

Void JxrInverseTransformNormalMacroblockInitialize(
    JxrInverseTransformNormalMacroblock* macroblock,
    const JxrInverseTransformNormalCodecSetup* setup,
    const JxrInverseTransformCodecInvocation* invocation)
{
    macroblock->geometry = &setup->geometry;
    macroblock->planePlan = &setup->planePlan;
    macroblock->postProcessParameters = &setup->postProcessParameters;
    macroblock->highPassParameters = &setup->highPassParameters;
    macroblock->firstStagePlanes = invocation->firstStagePlanes;
    macroblock->secondStagePlanes = invocation->secondStagePlanes;
    memcpy(macroblock->postProcessInfo, invocation->postProcessInfo,
        sizeof(macroblock->postProcessInfo));
    macroblock->macroblockColumn = setup->geometry.macroblockColumn;
    macroblock->usesScaledArithmetic = invocation->usesScaledArithmetic;
}

Void JxrInverseTransformNormalMacroblockProcess(JxrInverseTransformNormalMacroblock* macroblock)
{
    size_t channel;
    JxrInverseTransformPlaneContext plane;
    const JxrInverseTransformPlanePlan* plan = macroblock->planePlan;

    for (channel = 0; channel < plan->fullResolutionChannelCount && plan->transformsSamples; ++channel) {
        JxrInverseTransformPlaneContextInitialize(&plane, macroblock->firstStagePlanes,
            macroblock->secondStagePlanes, FALSE, channel, macroblock->postProcessParameters,
            macroblock->highPassParameters);
        JxrInverseTransformFullResolutionPlaneApply(&plane, macroblock->geometry,
            macroblock->usesScaledArithmetic, macroblock->postProcessParameters->enabled,
            macroblock->postProcessInfo, macroblock->macroblockColumn);
    }
    for (channel = 0; channel < plan->chroma420ChannelCount && plan->transformsSamples; ++channel) {
        JxrInverseTransformPlaneContextInitialize(&plane, macroblock->firstStagePlanes,
            macroblock->secondStagePlanes, TRUE, channel, macroblock->postProcessParameters,
            macroblock->highPassParameters);
        JxrInverseTransformChroma420PlaneApply(&plane, macroblock->geometry,
            macroblock->usesScaledArithmetic);
    }
    for (channel = 0; channel < plan->chroma422ChannelCount && plan->transformsSamples; ++channel) {
        JxrInverseTransformPlaneContextInitialize(&plane, macroblock->firstStagePlanes,
            macroblock->secondStagePlanes, TRUE, channel, macroblock->postProcessParameters,
            macroblock->highPassParameters);
        JxrInverseTransformChroma422PlaneApply(&plane, macroblock->geometry,
            macroblock->usesScaledArithmetic);
    }
}
