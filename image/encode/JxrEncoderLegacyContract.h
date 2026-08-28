#ifndef JXR_ENCODER_LEGACY_CONTRACT_H
#define JXR_ENCODER_LEGACY_CONTRACT_H

#include "strcodec.h"
#include "perfTimer.h"

Int ValidateArgs(CWMImageInfo* image, CWMIStrCodecParam* parameters);
Int StrEncInit(CWMImageStrCodec* codec);
Int WriteImagePlaneHeader(CWMImageStrCodec* codec);
Int writeIndexTableNull(CWMImageStrCodec* codec);

#endif
