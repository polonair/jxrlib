#ifndef JXR_ENCODER_OUTPUT_INITIALIZER_H
#define JXR_ENCODER_OUTPUT_INITIALIZER_H

#include "strcodec.h"

typedef struct JxrEncoderOutputPlan {
    Bool initializesPrimaryOutput;
    Bool sharesPrimaryOutput;
    Bool writesMainHeader;
} JxrEncoderOutputPlan;

Void JxrEncoderOutputPlanInitialize(JxrEncoderOutputPlan* plan, Bool isSecondary);
Int JxrEncoderOutputInitializerInitialize(CWMImageStrCodec* codec);

#endif
