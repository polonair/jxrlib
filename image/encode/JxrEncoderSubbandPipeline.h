#ifndef JXR_ENCODER_SUBBAND_PIPELINE_H
#define JXR_ENCODER_SUBBAND_PIPELINE_H

#include "strcodec.h"

/* Immutable subband selection for one encoded macroblock. */
typedef struct JxrEncoderSubbandPlan {
    Bool encodesLowpass;
    Bool encodesHighpass;
} JxrEncoderSubbandPlan;

Void JxrEncoderSubbandPlanInitialize(
    JxrEncoderSubbandPlan* plan,
    SUBBAND subband);

/* Encodes DC, then the enabled LP and HP subbands, including trace ranges. */
Int JxrEncoderSubbandPipelineProcess(
    CWMImageStrCodec* codec,
    CCodingContext* codingContext,
    Int macroblockX,
    Int macroblockY);

#endif
