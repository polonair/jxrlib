#include "JxrLowpassCbpState.h"

static Int JxrLowpassCbpStateClamp(Int value)
{
    if (value < -8) return -8;
    if (value > 7) return 7;
    return value;
}

Void JxrLowpassCbpStateInit(JxrLowpassCbpState* state, Int* zeroCount, Int* maxCount)
{
    state->zeroCount = zeroCount;
    state->maxCount = maxCount;
}

Int JxrLowpassCbpStateGetZeroCount(const JxrLowpassCbpState* state)
{
    return *state->zeroCount;
}

Int JxrLowpassCbpStateGetMaxCount(const JxrLowpassCbpState* state)
{
    return *state->maxCount;
}

Void JxrLowpassCbpStateObserve(JxrLowpassCbpState* state, Int cbp, Int maximumCbp)
{
    *state->maxCount = JxrLowpassCbpStateClamp(*state->maxCount + 1 -
        4 * (cbp == maximumCbp));
    *state->zeroCount = JxrLowpassCbpStateClamp(*state->zeroCount + 1 -
        4 * (cbp == 0));
}
