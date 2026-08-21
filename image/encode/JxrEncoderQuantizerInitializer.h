#ifndef JXR_ENCODER_QUANTIZER_INITIALIZER_H
#define JXR_ENCODER_QUANTIZER_INITIALIZER_H

#include "strcodec.h"

typedef struct JxrEncoderQuantizerPlan {
    Bool initializesDc;
    Bool initializesLp;
    Bool initializesHp;
} JxrEncoderQuantizerPlan;

Void JxrEncoderQuantizerPlanInitialize(JxrEncoderQuantizerPlan* plan,
    U32 quantizerMode, SUBBAND subband);
Int JxrEncoderQuantizerInitializerInitialize(CWMImageStrCodec* codec);

#endif
