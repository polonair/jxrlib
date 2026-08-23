#ifndef JXR_ENCODER_INPUT_ROW_PROCESSOR_H
#define JXR_ENCODER_INPUT_ROW_PROCESSOR_H

#include "strcodec.h"

typedef struct JxrEncoderInputRowPlan {
    Bool performsChromaDownsampling;
    Bool readsAlphaPlane;
} JxrEncoderInputRowPlan;

Void JxrEncoderInputRowPlanInitialize(JxrEncoderInputRowPlan* plan,
    Bool changesUvResolution, U8 alphaMode);
Int JxrEncoderInputRowProcessorProcess(CWMImageStrCodec* codec);

#endif
