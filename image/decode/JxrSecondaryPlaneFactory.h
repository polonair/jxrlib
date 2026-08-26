#ifndef JXR_SECONDARY_PLANE_FACTORY_H
#define JXR_SECONDARY_PLANE_FACTORY_H

#include "strcodec.h"

/* Creates and binds secondary alpha-plane storage from its explicit layout. */
Int JxrSecondaryPlaneFactoryCreate(const CCoreParameters* parameters,
    const CWMImageStrCodec* templateCodec, size_t channelBytes,
    size_t macroblockCount, CWMImageStrCodec** secondaryCodec);
Void JxrSecondaryPlaneFactoryRelease(CWMImageStrCodec* secondaryCodec);

#endif
