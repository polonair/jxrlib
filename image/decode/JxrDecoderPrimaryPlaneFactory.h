#ifndef JXR_DECODER_PRIMARY_PLANE_FACTORY_H
#define JXR_DECODER_PRIMARY_PLANE_FACTORY_H

#include "JxrDecoderMemoryLayoutPlan.h"

/* Creates and binds the primary decoder plane from its explicit layout. */
Int JxrDecoderPrimaryPlaneFactoryCreate(const JxrDecoderMemoryLayoutPlan* memoryLayout,
    const CCoreParameters* parameters, const CWMImageStrCodec* templateCodec,
    Bool usesHardTileBoundaries, Bool measuresPerformance,
    CWMImageStrCodec** primaryCodec);
Void JxrDecoderPrimaryPlaneFactoryRelease(CWMImageStrCodec* primaryCodec);

#endif
