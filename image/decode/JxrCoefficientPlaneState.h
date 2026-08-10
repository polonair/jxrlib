#ifndef JXR_COEFFICIENT_PLANE_STATE_H
#define JXR_COEFFICIENT_PLANE_STATE_H

#include "JxrCoefficientBuffer.h"
#include "strcodec.h"

/* Explicit macroblock plane references and bounds derived from the format snapshot. */
typedef struct JxrCoefficientPlaneState {
    PixelI* planes[MAX_CHANNELS];
    Int lengths[MAX_CHANNELS];
    Int planeCount;
} JxrCoefficientPlaneState;

Void JxrCoefficientPlaneStateInit(JxrCoefficientPlaneState* state, PixelI** nativePlanes,
    COLORFORMAT colorFormat, Int channelCount);
PixelI* JxrCoefficientPlaneStateGetPlane(const JxrCoefficientPlaneState* state, Int plane);
Int JxrCoefficientPlaneStateGetLength(const JxrCoefficientPlaneState* state, Int plane);
JxrCoefficientBuffer JxrCoefficientPlaneStateGetBlock(const JxrCoefficientPlaneState* state,
    Int plane, Int offset, Int count);

#endif
