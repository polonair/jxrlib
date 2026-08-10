#include "JxrEntropyReader.h"

Void JxrEntropyBitReaderInit(JxrEntropyBitReader* state, BitIOInfo* legacyStream)
{
    state->legacyStream = legacyStream;
}

U32 JxrEntropyBitReaderPeek(JxrEntropyBitReader* state, U32 count)
{
    return peekBit16(state->legacyStream, count);
}

Void JxrEntropyBitReaderConsume(JxrEntropyBitReader* state, U32 count)
{
    flushBit16(state->legacyStream, count);
}

U32 JxrEntropyBitReaderRead(JxrEntropyBitReader* state, U32 count)
{
    return getBit16(state->legacyStream, count);
}

U32 JxrEntropyBitReaderReadFlag(JxrEntropyBitReader* state)
{
    return JxrEntropyBitReaderRead(state, 1);
}

I32 JxrEntropyBitReaderReadSign(JxrEntropyBitReader* state)
{
    return JxrEntropyBitReaderReadFlag(state) ? -1 : 0;
}

I32 JxrEntropyBitReaderDecodeSignedResidualValue(U32 encodedValue)
{
    I32 magnitude = (I32)(encodedValue >> 1);
    if (magnitude == 0)
        return 0;
    return (encodedValue & 1) ? -magnitude : magnitude;
}

I32 JxrEntropyBitReaderReadSignedResidual(JxrEntropyBitReader* state, U32 count)
{
    I32 value = JxrEntropyBitReaderDecodeSignedResidualValue(
        JxrEntropyBitReaderPeek(state, count + 1));
    JxrEntropyBitReaderConsume(state, count + (value != 0));
    return value;
}

U32 JxrEntropyReaderPeek(BitIOInfo* state, U32 count)
{
    JxrEntropyBitReader reader;
    JxrEntropyBitReaderInit(&reader, state);
    return JxrEntropyBitReaderPeek(&reader, count);
}

Void JxrEntropyReaderConsume(BitIOInfo* state, U32 count)
{
    JxrEntropyBitReader reader;
    JxrEntropyBitReaderInit(&reader, state);
    JxrEntropyBitReaderConsume(&reader, count);
}

U32 JxrEntropyReaderRead(BitIOInfo* state, U32 count)
{
    JxrEntropyBitReader reader;
    JxrEntropyBitReaderInit(&reader, state);
    return JxrEntropyBitReaderRead(&reader, count);
}

U32 JxrEntropyReaderReadFlag(BitIOInfo* state)
{
    JxrEntropyBitReader reader;
    JxrEntropyBitReaderInit(&reader, state);
    return JxrEntropyBitReaderReadFlag(&reader);
}

I32 JxrEntropyReaderReadSign(BitIOInfo* state)
{
    JxrEntropyBitReader reader;
    JxrEntropyBitReaderInit(&reader, state);
    return JxrEntropyBitReaderReadSign(&reader);
}

I32 JxrEntropyReaderReadSignedResidual(BitIOInfo* state, U32 count)
{
    JxrEntropyBitReader reader;
    JxrEntropyBitReaderInit(&reader, state);
    return JxrEntropyBitReaderReadSignedResidual(&reader, count);
}
