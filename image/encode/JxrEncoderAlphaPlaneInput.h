#ifndef JXR_ENCODER_ALPHA_PLANE_INPUT_H
#define JXR_ENCODER_ALPHA_PLANE_INPUT_H

#include "strcodec.h"

typedef struct JxrEncoderAlphaPlaneInputPlan {
    Bool readsAlphaPlane;
    Bool supportsSourceBitDepth;
} JxrEncoderAlphaPlaneInputPlan;

Void JxrEncoderAlphaPlaneInputPlanInitialize(JxrEncoderAlphaPlaneInputPlan* plan,
    Bool isSecondaryCodec, Bool hasSecondaryCodec, BITDEPTH_BITS sourceBitDepth);
Int JxrEncoderAlphaPlaneInputRead(CWMImageStrCodec* codec);

#endif
