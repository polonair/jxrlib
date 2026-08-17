#ifndef JXR_INVERSE_TRANSFORM_CODEC_INVOCATION_H
#define JXR_INVERSE_TRANSFORM_CODEC_INVOCATION_H

#include "JxrInverseTransformMacroblockGeometry.h"

/* Mutable legacy buffers exposed as explicit inputs to one macroblock transform. */
typedef struct JxrInverseTransformCodecInvocation {
    PixelI* const* firstStagePlanes;
    PixelI* const* secondStagePlanes;
    struct tagPostProcInfo* (*postProcessInfo)[2];
    PixelI (*predictionBefore)[2];
    PixelI (*predictionAfter)[2];
    size_t channelCount;
    Bool usesScaledArithmetic;
} JxrInverseTransformCodecInvocation;

Void JxrInverseTransformCodecInvocationInitialize(
    JxrInverseTransformCodecInvocation* invocation,
    CWMImageStrCodec* codec);

Void JxrInverseTransformCodecInvocationAdvancePostProcessRow(
    JxrInverseTransformCodecInvocation* invocation,
    const JxrInverseTransformMacroblockGeometry* geometry,
    Bool postProcessEnabled);

#endif
