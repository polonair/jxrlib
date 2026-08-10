#include "JxrBitCursorState.h"

#include <assert.h>

static size_t JxrBitCursorStateWrapIndex(const JxrBitCursorState* state, size_t index)
{
    return index % state->length;
}

static U32 JxrBitCursorStateLoad32BigEndian(const JxrBitCursorState* state)
{
    size_t index = state->currentIndex;
    return ((U32)state->buffer[index] << 24) |
        ((U32)state->buffer[JxrBitCursorStateWrapIndex(state, index + 1)] << 16) |
        ((U32)state->buffer[JxrBitCursorStateWrapIndex(state, index + 2)] << 8) |
        (U32)state->buffer[JxrBitCursorStateWrapIndex(state, index + 3)];
}

Void JxrBitCursorStateInit(JxrBitCursorState* state, const U8* buffer, size_t length,
    size_t currentIndex, U32 accumulator, U32 usedBits)
{
    assert(buffer != NULL && length != 0 && currentIndex < length);
    state->buffer = buffer;
    state->length = length;
    state->currentIndex = currentIndex;
    state->accumulator = accumulator;
    state->usedBits = usedBits;
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
    state->currentIndex = JxrBitCursorStateWrapIndex(state,
        state->currentIndex + (state->usedBits >> 3));
    state->currentIndex &= ~(size_t)1;
    state->usedBits &= 15;
    state->accumulator = JxrBitCursorStateLoad32BigEndian(state) << state->usedBits;
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
