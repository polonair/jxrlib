#ifndef JXR_BIT_CURSOR_STATE_H
#define JXR_BIT_CURSOR_STATE_H

#include "windowsmediaphoto.h"

/* Explicit counterpart of the decoder's legacy BitIOInfo read cursor. */
typedef struct JxrBitCursorState {
    const U8* buffer;
    size_t length;
    size_t currentIndex;
    U32 accumulator;
    U32 usedBits;
} JxrBitCursorState;

Void JxrBitCursorStateInit(JxrBitCursorState* state, const U8* buffer, size_t length,
    size_t currentIndex, U32 accumulator, U32 usedBits);
U32 JxrBitCursorStatePeek(const JxrBitCursorState* state, U32 count);
Void JxrBitCursorStateConsume(JxrBitCursorState* state, U32 count);
U32 JxrBitCursorStateReadLong(JxrBitCursorState* state, U32 count);

#endif
