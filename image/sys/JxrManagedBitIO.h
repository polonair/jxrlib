#ifndef JXR_MANAGED_BIT_IO_H
#define JXR_MANAGED_BIT_IO_H

#include "windowsmediaphoto.h"

typedef struct JxrBitWriter {
    U8* buffer;
    size_t capacity;
    size_t byteIndex;
    U32 accumulator;
    U32 bitCount;
    Bool failed;
} JxrBitWriter;

typedef struct JxrBitReader {
    const U8* buffer;
    size_t length;
    size_t byteIndex;
    U32 accumulator;
    U32 bitCount;
    Bool failed;
} JxrBitReader;

Void JxrBitWriterInit(JxrBitWriter* state, U8* buffer, size_t capacity);
Bool JxrBitWriterWrite(JxrBitWriter* state, U32 value, U32 count);
Bool JxrBitWriterFlush(JxrBitWriter* state);
size_t JxrBitWriterBytes(const JxrBitWriter* state);
Void JxrBitReaderInit(JxrBitReader* state, const U8* buffer, size_t length);
Bool JxrBitReaderRead(JxrBitReader* state, U32 count, U32* value);

#endif
