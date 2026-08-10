#include "JxrCoefficientPlaneState.h"

Void JxrCoefficientPlaneStateInit(JxrCoefficientPlaneState* state, PixelI** nativePlanes)
{
    state->nativePlanes = nativePlanes;
}

PixelI* JxrCoefficientPlaneStateGetPlane(const JxrCoefficientPlaneState* state, Int plane)
{
    return state->nativePlanes[plane];
}

JxrCoefficientBuffer JxrCoefficientPlaneStateGetBlock(const JxrCoefficientPlaneState* state,
    Int plane, size_t offset, size_t count)
{
    return JxrCoefficientBufferCreate(JxrCoefficientPlaneStateGetPlane(state, plane), offset, count);
}
