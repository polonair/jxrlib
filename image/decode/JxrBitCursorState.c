#include "JxrBitCursorState.h"

static U32 JxrBitCursorStateLoad32BigEndian(const U8* bytes)
{
    return ((U32)bytes[0] << 24) | ((U32)bytes[1] << 16) |
        ((U32)bytes[2] << 8) | (U32)bytes[3];
}

Void JxrBitCursorStateInit(JxrBitCursorState* state, const BitIOInfo* legacy)
{
    state->currentAddress = (UINTPTR_T)legacy->pbCurrent;
    state->mask = (UINTPTR_T)legacy->iMask;
    state->accumulator = legacy->uiAccumulator;
    state->usedBits = legacy->cBitsUsed;
}

U32 JxrBitCursorStatePeek(const JxrBitCursorState* state, U32 count)
{
    assert(count <= 16);
    if (count == 0)
        return 0;
    return state->accumulator >> (32 - count);
}

Void JxrBitCursorStateConsume(JxrBitCursorState* state, U32 count)
{
    assert(count <= 16);
    if (count == 0)
        return;
    state->usedBits += count;
    state->currentAddress = ((state->currentAddress + (state->usedBits >> 3)) & state->mask);
    state->usedBits &= 15;
    state->accumulator = JxrBitCursorStateLoad32BigEndian(
        (const U8*)state->currentAddress) << state->usedBits;
}

U32 JxrBitCursorStateReadLong(JxrBitCursorState* state, U32 count)
{
    U32 value = 0;
    assert(count <= 32);
    if (count == 0)
        return 0;
    if (count > 16) {
        value = JxrBitCursorStatePeek(state, 16);
        JxrBitCursorStateConsume(state, 16);
        value <<= count - 16;
        count -= 16;
    }
    value |= JxrBitCursorStatePeek(state, count);
    JxrBitCursorStateConsume(state, count);
    return value;
}

Bool JxrBitCursorStateMatchesLegacy(const JxrBitCursorState* state, const BitIOInfo* legacy)
{
    return state->currentAddress == (UINTPTR_T)legacy->pbCurrent &&
        state->mask == (UINTPTR_T)legacy->iMask &&
        state->accumulator == legacy->uiAccumulator &&
        state->usedBits == legacy->cBitsUsed;
}
