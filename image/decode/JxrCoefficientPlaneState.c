#include "JxrCoefficientPlaneState.h"

static Int JxrCoefficientPlaneStateGetFormatLength(COLORFORMAT colorFormat, Int plane)
{
    if (plane == 0 || (colorFormat != YUV_420 && colorFormat != YUV_422))
        return 256;
    return colorFormat == YUV_420 ? 64 : 128;
}

Void JxrCoefficientPlaneStateInit(JxrCoefficientPlaneState* state, PixelI** nativePlanes,
    COLORFORMAT colorFormat, Int channelCount)
{
    Int plane;
    assert(channelCount >= 0 && channelCount <= MAX_CHANNELS);
    state->planeCount = channelCount;
    for (plane = 0; plane < MAX_CHANNELS; ++plane) {
        state->planes[plane] = NULL;
        state->lengths[plane] = 0;
    }
    for (plane = 0; plane < channelCount; ++plane) {
        state->planes[plane] = nativePlanes[plane];
        state->lengths[plane] = JxrCoefficientPlaneStateGetFormatLength(colorFormat, plane);
    }
}

PixelI* JxrCoefficientPlaneStateGetPlane(const JxrCoefficientPlaneState* state, Int plane)
{
    assert(plane >= 0 && plane < state->planeCount);
    return state->planes[plane];
}

Int JxrCoefficientPlaneStateGetLength(const JxrCoefficientPlaneState* state, Int plane)
{
    assert(plane >= 0 && plane < state->planeCount);
    return state->lengths[plane];
}

JxrCoefficientBuffer JxrCoefficientPlaneStateGetBlock(const JxrCoefficientPlaneState* state,
    Int plane, Int offset, Int count)
{
    assert(offset >= 0 && count >= 0);
    assert(offset <= JxrCoefficientPlaneStateGetLength(state, plane));
    assert(count <= JxrCoefficientPlaneStateGetLength(state, plane) - offset);
    return JxrCoefficientBufferCreate(JxrCoefficientPlaneStateGetPlane(state, plane), offset, count);
}
