#include "JxrManagedBitIO.h"

Void JxrBitWriterInit(JxrBitWriter* s, U8* buffer, size_t capacity)
{ s->buffer = buffer; s->capacity = capacity; s->byteIndex = 0; s->accumulator = 0; s->bitCount = 0; s->failed = FALSE; }

Bool JxrBitWriterWrite(JxrBitWriter* s, U32 value, U32 count)
{
    if (count > 32 || (count < 32 && (value >> count) != 0)) { s->failed = TRUE; return FALSE; }
    while (count) {
        U32 take = count > 8 - s->bitCount ? 8 - s->bitCount : count;
        U32 shift = count - take;
        s->accumulator = (s->accumulator << take) | ((value >> shift) & ((1U << take) - 1));
        s->bitCount += take; count -= take;
        if (s->bitCount == 8) { if (s->byteIndex == s->capacity) { s->failed = TRUE; return FALSE; } s->buffer[s->byteIndex++] = (U8)s->accumulator; s->accumulator = 0; s->bitCount = 0; }
    }
    return TRUE;
}

Bool JxrBitWriterFlush(JxrBitWriter* s)
{ if (s->bitCount) { if (s->byteIndex == s->capacity) { s->failed=TRUE; return FALSE; } s->buffer[s->byteIndex++] = (U8)(s->accumulator << (8-s->bitCount)); s->accumulator=0; s->bitCount=0; } return !s->failed; }
size_t JxrBitWriterBytes(const JxrBitWriter* s) { return s->byteIndex; }
Void JxrBitReaderInit(JxrBitReader* s, const U8* b, size_t n)
{ s->buffer=b; s->length=n; s->byteIndex=0; s->accumulator=0; s->bitCount=0; s->failed=FALSE; }
Bool JxrBitReaderRead(JxrBitReader* s, U32 count, U32* value)
{
    U32 result=0; if (count>32) { s->failed=TRUE; return FALSE; }
    while(count) { U32 take; if(!s->bitCount) { if(s->byteIndex==s->length) {s->failed=TRUE;return FALSE;} s->accumulator=s->buffer[s->byteIndex++];s->bitCount=8; } take=count<s->bitCount?count:s->bitCount; result=(result<<take)|((s->accumulator>>(s->bitCount-take))&((1U<<take)-1)); s->bitCount-=take; count-=take; }
    *value=result; return TRUE;
}
