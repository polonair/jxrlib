#ifndef JXR_ENCODER_IMAGE_PLANE_HEADER_WRITER_H
#define JXR_ENCODER_IMAGE_PLANE_HEADER_WRITER_H

#include "strcodec.h"

/* Immutable frame-quantizer selection for one image plane header. */
typedef struct JxrEncoderImagePlaneHeaderQuantizerPlan {
    Bool writesDcFrameQuantizer;
    Bool writesLowpassSyntax;
    Bool lowpassUsesDcQuantizer;
    Bool writesLpFrameQuantizer;
    Bool writesHighpassSyntax;
    Bool highpassUsesLpQuantizer;
    Bool writesHpFrameQuantizer;
} JxrEncoderImagePlaneHeaderQuantizerPlan;

Void JxrEncoderImagePlaneHeaderQuantizerPlanInitialize(
    JxrEncoderImagePlaneHeaderQuantizerPlan* plan,
    U32 quantizerMode,
    SUBBAND subband);

Int JxrEncoderImagePlaneHeaderWriterWrite(CWMImageStrCodec* codec);

#endif
