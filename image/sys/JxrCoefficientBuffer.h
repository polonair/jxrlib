#ifndef JXR_COEFFICIENT_BUFFER_H
#define JXR_COEFFICIENT_BUFFER_H

#include <stddef.h>

#include "windowsmediaphoto.h"

/*
 * A bounded view over one coefficient array.  The explicit offset mirrors the
 * future managed representation: one array plus an index, rather than a C
 * pointer that is advanced through a block.
 */
typedef struct JxrCoefficientBuffer {
    PixelI* values;
    size_t offset;
    size_t count;
} JxrCoefficientBuffer;

JxrCoefficientBuffer JxrCoefficientBufferCreate(PixelI* values, size_t offset, size_t count);
PixelI JxrCoefficientBufferGet(const JxrCoefficientBuffer* buffer, size_t index);
Void JxrCoefficientBufferSet(JxrCoefficientBuffer* buffer, size_t index, PixelI value);
Void JxrCoefficientBufferAdd(JxrCoefficientBuffer* buffer, size_t index, PixelI value);
Void JxrCoefficientBufferClear(JxrCoefficientBuffer* buffer);

#endif
