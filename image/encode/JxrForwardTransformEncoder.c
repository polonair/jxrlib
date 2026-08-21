#include "JxrForwardTransformEncoder.h"

#include "JxrForwardTransformChroma420Plane.h"
#include "JxrForwardTransformChroma422Plane.h"
#include "JxrForwardTransformCodecSetup.h"
#include "JxrForwardTransformFullResolutionPlane.h"
#include "JxrForwardTransformPlaneContext.h"

Void JxrForwardTransformEncoderProcessMacroblock(CWMImageStrCodec* codec)
{
    JxrForwardTransformCodecSetup setup;
    JxrForwardTransformPlaneContext plane;
    Int channelIndex;

    JxrForwardTransformCodecSetupInitialize(&setup, codec);

    for (channelIndex = 0;
        channelIndex < (Int)setup.planePlan.fullResolutionChannelCount;
        ++channelIndex) {
        JxrForwardTransformPlaneContextInitializeFullResolution(&plane,
            codec->p0MBbuffer, codec->p1MBbuffer, channelIndex);
        JxrForwardTransformFullResolutionPlaneApply(
            plane.firstStage, plane.secondStage, plane.channelIndex != 0,
            &setup.geometry, &setup.boundaries,
            setup.usesScaledArithmetic);
    }

    for (channelIndex = 0;
        channelIndex < (Int)setup.planePlan.chroma420ChannelCount;
        ++channelIndex) {
        JxrForwardTransformPlaneContextInitializeChroma(&plane,
            codec->p0MBbuffer, codec->p1MBbuffer,
            codec->iPredBefore, codec->iPredAfter, channelIndex);
        JxrForwardTransformChroma420PlaneApply(
            plane.firstStage, plane.secondStage, plane.predictionBefore, plane.predictionAfter,
            &setup.geometry, &setup.boundaries, setup.usesScaledArithmetic);
    }

    for (channelIndex = 0;
        channelIndex < (Int)setup.planePlan.chroma422ChannelCount;
        ++channelIndex) {
        JxrForwardTransformPlaneContextInitializeChroma(&plane,
            codec->p0MBbuffer, codec->p1MBbuffer,
            codec->iPredBefore, codec->iPredAfter, channelIndex);
        JxrForwardTransformChroma422PlaneApply(
            plane.firstStage, plane.secondStage, plane.predictionBefore, plane.predictionAfter,
            &setup.geometry, &setup.boundaries, setup.usesScaledArithmetic);
    }
}
