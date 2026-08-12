#ifndef JXR_SECONDARY_PLANE_INITIALIZER_H
#define JXR_SECONDARY_PLANE_INITIALIZER_H

#include "JxrDecoderInitializationPipeline.h"

typedef Void (*JxrSecondaryPlaneCodecInitializer)(CWMImageStrCodec* codec,
    const CCoreParameters* parameters, const CWMImageStrCodec* templateCodec);
typedef Int (*JxrSecondaryPlaneHeaderReader)(CWMImageInfo* imageInfo,
    CWMIStrCodecParam* codecParameters, CCoreParameters* coreParameters,
    SimpleBitIO* bitInput);

typedef struct JxrSecondaryPlaneInitializer {
    CWMImageStrCodec* primaryCodec;
    const CCoreParameters* templateParameters;
    const CWMImageStrCodec* templateCodec;
    size_t channelBytes;
    size_t macroblockCount;
    JxrSecondaryPlaneCodecInitializer initializeCodec;
    JxrSecondaryPlaneHeaderReader readHeader;
} JxrSecondaryPlaneInitializer;

Void JxrSecondaryPlaneInitializerInit(JxrSecondaryPlaneInitializer* initializer,
    CWMImageStrCodec* primaryCodec, const CCoreParameters* templateParameters,
    const CWMImageStrCodec* templateCodec, size_t channelBytes, size_t macroblockCount,
    JxrSecondaryPlaneCodecInitializer initializeCodec,
    JxrSecondaryPlaneHeaderReader readHeader);
Int JxrSecondaryPlaneInitializerRun(JxrSecondaryPlaneInitializer* initializer,
    CWMImageStrCodec** secondaryCodec);

#endif
