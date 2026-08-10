#ifndef JXR_COEFFICIENT_BUFFER_H
#define JXR_COEFFICIENT_BUFFER_H

#include "windowsmediaphoto.h"

/*
 * A bounded view over one coefficient array.  The explicit offset mirrors the
 * future managed representation: one array plus an index, rather than a C
 * pointer that is advanced through a block.
 */
typedef struct JxrCoefficientBuffer {
    PixelI* values;
    Int offset;
    Int count;
} JxrCoefficientBuffer;

JxrCoefficientBuffer JxrCoefficientBufferCreate(PixelI* values, Int offset, Int count);
PixelI JxrCoefficientBufferGet(const JxrCoefficientBuffer* buffer, Int index);
Void JxrCoefficientBufferSet(JxrCoefficientBuffer* buffer, Int index, PixelI value);
Void JxrCoefficientBufferAdd(JxrCoefficientBuffer* buffer, Int index, PixelI value);
Void JxrCoefficientBufferClear(JxrCoefficientBuffer* buffer);

#endif
