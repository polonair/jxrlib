#ifndef JXR_COEFFICIENT_PLANE_STATE_H
#define JXR_COEFFICIENT_PLANE_STATE_H

#include "JxrCoefficientBuffer.h"
#include "strcodec.h"

/* Explicit macroblock plane references and bounds derived from the format snapshot. */
typedef struct JxrCoefficientPlaneState {
    PixelI* planes[MAX_CHANNELS];
    size_t lengths[MAX_CHANNELS];
    Int planeCount;
} JxrCoefficientPlaneState;

Void JxrCoefficientPlaneStateInit(JxrCoefficientPlaneState* state, PixelI** nativePlanes,
    COLORFORMAT colorFormat, Int channelCount);
PixelI* JxrCoefficientPlaneStateGetPlane(const JxrCoefficientPlaneState* state, Int plane);
size_t JxrCoefficientPlaneStateGetLength(const JxrCoefficientPlaneState* state, Int plane);
JxrCoefficientBuffer JxrCoefficientPlaneStateGetBlock(const JxrCoefficientPlaneState* state,
    Int plane, size_t offset, size_t count);

#endif
