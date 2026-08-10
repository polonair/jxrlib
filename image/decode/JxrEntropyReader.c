#include "JxrEntropyReader.h"

Void JxrEntropyBitReaderInit(JxrEntropyBitReader* state, BitIOInfo* legacyStream)
{
    JxrLegacyBitReaderAdapterInit(&state->adapter, legacyStream);
    state->positionBits = 0;
    state->hasError = FALSE;
}

U32 JxrEntropyBitReaderPeek(JxrEntropyBitReader* state, U32 count)
{
    if (count > 16) {
        state->hasError = TRUE;
        return 0;
    }
    return JxrLegacyBitReaderAdapterPeek16(&state->adapter, count);
}

Void JxrEntropyBitReaderConsume(JxrEntropyBitReader* state, U32 count)
{
    if (count > 16) {
        state->hasError = TRUE;
        return;
    }
    JxrLegacyBitReaderAdapterConsume16(&state->adapter, count);
    state->positionBits += count;
}

U32 JxrEntropyBitReaderRead(JxrEntropyBitReader* state, U32 count)
{
    U32 value = JxrEntropyBitReaderPeek(state, count);
    JxrEntropyBitReaderConsume(state, count);
    return value;
}

U32 JxrEntropyBitReaderReadLong(JxrEntropyBitReader* state, U32 count)
{
    if (count > 32) {
        state->hasError = TRUE;
        return 0;
    }
    state->positionBits += count;
    return JxrLegacyBitReaderAdapterRead32(&state->adapter, count);
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

U32 JxrEntropyBitReaderPosition(const JxrEntropyBitReader* state)
{
    return state->positionBits;
}

Bool JxrEntropyBitReaderHasError(const JxrEntropyBitReader* state)
{
    return state->hasError;
}

Bool JxrEntropyBitReaderSharesStream(const JxrEntropyBitReader* left,
    const JxrEntropyBitReader* right)
{
    return JxrLegacyBitReaderAdapterSharesStream(&left->adapter, &right->adapter);
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
