#ifndef JXR_DECODER_SESSION_FACTORY_H
#define JXR_DECODER_SESSION_FACTORY_H

#include "JxrDecoderSessionPreparation.h"

/* Builds a ready decoder session from validated, prepared input state. */
Int JxrDecoderSessionFactoryCreate(const JxrDecoderSessionPreparation* preparation,
    Bool measuresPerformance, CWMImageInfo* imageInfo,
    CWMIStrCodecParam* codecParameters, CWMImageStrCodec** primaryCodec);

#endif
