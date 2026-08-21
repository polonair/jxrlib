#include "JxrForwardTransformEncoder.h"

#include "JxrForwardTransformChroma420Plane.h"
#include "JxrForwardTransformChroma422Plane.h"
#include "JxrForwardTransformCodecSetup.h"
#include "JxrForwardTransformFullResolutionPlane.h"

Void JxrForwardTransformEncoderProcessMacroblock(CWMImageStrCodec* codec)
{
    JxrForwardTransformCodecSetup setup;
    Int channelIndex;

    JxrForwardTransformCodecSetupInitialize(&setup, codec);

    for (channelIndex = 0;
        channelIndex < (Int)setup.geometry.fullResolutionPlaneCount;
        ++channelIndex) {
        JxrForwardTransformFullResolutionPlaneApply(
            codec->p0MBbuffer[channelIndex], codec->p1MBbuffer[channelIndex],
            channelIndex != 0, &setup.geometry, &setup.boundaries,
            setup.usesScaledArithmetic);
    }

    for (channelIndex = 0;
        channelIndex < (setup.geometry.colorFormat == YUV_420 ? 2 : 0);
        ++channelIndex) {
        JxrForwardTransformChroma420PlaneApply(
            codec->p0MBbuffer[1 + channelIndex], codec->p1MBbuffer[1 + channelIndex],
            codec->iPredBefore[channelIndex], codec->iPredAfter[channelIndex],
            &setup.geometry, &setup.boundaries, setup.usesScaledArithmetic);
    }

    for (channelIndex = 0;
        channelIndex < (setup.geometry.colorFormat == YUV_422 ? 2 : 0);
        ++channelIndex) {
        JxrForwardTransformChroma422PlaneApply(
            codec->p0MBbuffer[1 + channelIndex], codec->p1MBbuffer[1 + channelIndex],
            codec->iPredBefore[channelIndex], codec->iPredAfter[channelIndex],
            &setup.geometry, &setup.boundaries, setup.usesScaledArithmetic);
    }
}
