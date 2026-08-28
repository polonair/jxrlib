#ifndef JXR_ENCODER_SESSION_FACTORY_H
#define JXR_ENCODER_SESSION_FACTORY_H

#include "strcodec.h"

Int JxrEncoderSessionFactoryCreate(CWMImageInfo* image,
    CWMIStrCodecParam* parameters, CTXSTRCODEC* context);

#endif
