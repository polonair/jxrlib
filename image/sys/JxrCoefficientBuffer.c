#include "JxrCoefficientBuffer.h"

#include <assert.h>

JxrCoefficientBuffer JxrCoefficientBufferCreate(PixelI* values, Int offset, Int count)
{
    JxrCoefficientBuffer buffer;
    assert(values != NULL && offset >= 0 && count >= 0);
    buffer.values = values;
    buffer.offset = offset;
    buffer.count = count;
    return buffer;
}

PixelI JxrCoefficientBufferGet(const JxrCoefficientBuffer* buffer, Int index)
{
    assert(buffer != NULL && index >= 0 && index < buffer->count);
    return buffer->values[buffer->offset + index];
}

Void JxrCoefficientBufferSet(JxrCoefficientBuffer* buffer, Int index, PixelI value)
{
    assert(buffer != NULL && index >= 0 && index < buffer->count);
    buffer->values[buffer->offset + index] = value;
}

Void JxrCoefficientBufferAdd(JxrCoefficientBuffer* buffer, Int index, PixelI value)
{
    assert(buffer != NULL && index >= 0 && index < buffer->count);
    buffer->values[buffer->offset + index] += value;
}

Void JxrCoefficientBufferClear(JxrCoefficientBuffer* buffer)
{
    Int index;
    for (index = 0; index < buffer->count; ++index) {
        JxrCoefficientBufferSet(buffer, index, 0);
    }
}
