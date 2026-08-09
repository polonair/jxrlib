#include "JxrEntropyReader.h"

U32 JxrEntropyReaderPeek(BitIOInfo* state, U32 count) { return peekBit16(state, count); }
Void JxrEntropyReaderConsume(BitIOInfo* state, U32 count) { flushBit16(state, count); }
U32 JxrEntropyReaderRead(BitIOInfo* state, U32 count) { return getBit16(state, count); }
U32 JxrEntropyReaderReadFlag(BitIOInfo* state) { return getBit16(state, 1); }
I32 JxrEntropyReaderReadSign(BitIOInfo* state) { return -(I32)getBit16(state, 1); }
I32 JxrEntropyReaderReadSignedResidual(BitIOInfo* state, U32 count)
{
    I32 value = (I32)peekBit16(state, count + 1);
    value = ((value >> 1) ^ -(value & 1)) + (value & 1);
    flushBit16(state, count + (value != 0));
    return value;
}
