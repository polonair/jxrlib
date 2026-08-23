#ifndef JXR_ENCODER_INPUT_PADDING_H
#define JXR_ENCODER_INPUT_PADDING_H

#include "strcodec.h"

typedef struct JxrEncoderInputPaddingPlan {
    Bool requiresPadding;
    COLORFORMAT sourceFormat;
    size_t fullResolutionChannelCount;
    Bool padsYuv422Chroma;
    Bool padsYuv420Chroma;
} JxrEncoderInputPaddingPlan;

Void JxrEncoderInputPaddingPlanInitialize(JxrEncoderInputPaddingPlan* plan,
    size_t imageWidth, size_t macroblockWidth, Bool inputIsYuvData,
    COLORFORMAT sourceFormat, COLORFORMAT targetFormat, size_t channelCount);
Void JxrEncoderInputPaddingApply(CWMImageStrCodec* codec);

#endif
