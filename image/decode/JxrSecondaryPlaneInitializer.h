#ifndef JXR_SECONDARY_PLANE_INITIALIZER_H
#define JXR_SECONDARY_PLANE_INITIALIZER_H

#include "JxrDecoderInitializationPipeline.h"

typedef struct JxrSecondaryPlaneInitializer {
    CWMImageStrCodec* primaryCodec;
    const CCoreParameters* templateParameters;
    const CWMImageStrCodec* templateCodec;
    size_t channelBytes;
    size_t macroblockCount;
} JxrSecondaryPlaneInitializer;

Void JxrSecondaryPlaneInitializerInit(JxrSecondaryPlaneInitializer* initializer,
    CWMImageStrCodec* primaryCodec, const CCoreParameters* templateParameters,
    const CWMImageStrCodec* templateCodec, size_t channelBytes, size_t macroblockCount);
Int JxrSecondaryPlaneInitializerRun(JxrSecondaryPlaneInitializer* initializer,
    CWMImageStrCodec** secondaryCodec);

#endif
