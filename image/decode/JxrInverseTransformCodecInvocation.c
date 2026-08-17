#include "strTransform.h"
#include "strcodec.h"
#include "decode.h"
#include "JxrInverseTransformCodecInvocation.h"

Void JxrInverseTransformCodecInvocationInitialize(
    JxrInverseTransformCodecInvocation* invocation,
    CWMImageStrCodec* codec)
{
    invocation->firstStagePlanes = codec->p0MBbuffer;
    invocation->secondStagePlanes = codec->p1MBbuffer;
    invocation->postProcessInfo = codec->pPostProcInfo;
    invocation->predictionBefore = codec->iPredBefore;
    invocation->predictionAfter = codec->iPredAfter;
    invocation->channelCount = codec->m_param.cNumChannels;
    invocation->usesScaledArithmetic = codec->m_param.bScaledArith;
}

Void JxrInverseTransformCodecInvocationAdvancePostProcessRow(
    JxrInverseTransformCodecInvocation* invocation,
    const JxrInverseTransformMacroblockGeometry* geometry,
    Bool postProcessEnabled)
{
    if (postProcessEnabled && geometry->isLeft)
        slideOneMBRow(invocation->postProcessInfo, invocation->channelCount,
            geometry->macroblockWidth, geometry->isTop, geometry->isBottom);
}
