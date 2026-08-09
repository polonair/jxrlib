#include "JxrCoefficientBuffer.h"

JxrCoefficientBuffer JxrCoefficientBufferCreate(PixelI* values, size_t offset, size_t count)
{
    JxrCoefficientBuffer buffer;
    buffer.values = values;
    buffer.offset = offset;
    buffer.count = count;
    return buffer;
}

PixelI JxrCoefficientBufferGet(const JxrCoefficientBuffer* buffer, size_t index)
{
    return buffer->values[buffer->offset + index];
}

Void JxrCoefficientBufferSet(JxrCoefficientBuffer* buffer, size_t index, PixelI value)
{
    buffer->values[buffer->offset + index] = value;
}

Void JxrCoefficientBufferAdd(JxrCoefficientBuffer* buffer, size_t index, PixelI value)
{
    buffer->values[buffer->offset + index] += value;
}

Void JxrCoefficientBufferClear(JxrCoefficientBuffer* buffer)
{
    size_t index;
    for (index = 0; index < buffer->count; ++index) {
        JxrCoefficientBufferSet(buffer, index, 0);
    }
}
