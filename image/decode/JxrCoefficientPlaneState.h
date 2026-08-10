#ifndef JXR_COEFFICIENT_PLANE_STATE_H
#define JXR_COEFFICIENT_PLANE_STATE_H

#include "JxrCoefficientBuffer.h"

/* Explicit access to macroblock coefficient planes. */
typedef struct JxrCoefficientPlaneState {
    PixelI** nativePlanes;
} JxrCoefficientPlaneState;

Void JxrCoefficientPlaneStateInit(JxrCoefficientPlaneState* state, PixelI** nativePlanes);
PixelI* JxrCoefficientPlaneStateGetPlane(const JxrCoefficientPlaneState* state, Int plane);
JxrCoefficientBuffer JxrCoefficientPlaneStateGetBlock(const JxrCoefficientPlaneState* state,
    Int plane, size_t offset, size_t count);

#endif
