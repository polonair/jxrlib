#include "JxrLegacyBitReaderAdapter.h"

Void JxrLegacyBitReaderAdapterInit(JxrLegacyBitReaderAdapter* state, BitIOInfo* stream)
{
    state->stream = stream;
}

U32 JxrLegacyBitReaderAdapterPeek16(JxrLegacyBitReaderAdapter* state, U32 count)
{
    return peekBit16(state->stream, count);
}

Void JxrLegacyBitReaderAdapterConsume16(JxrLegacyBitReaderAdapter* state, U32 count)
{
    flushBit16(state->stream, count);
}

U32 JxrLegacyBitReaderAdapterRead32(JxrLegacyBitReaderAdapter* state, U32 count)
{
    return getBit32(state->stream, count);
}

Void JxrLegacyBitReaderAdapterRefillLevel1(CWMImageStrCodec* codec, JxrLegacyBitReaderAdapter* state)
{
    readIS_L1(codec, state->stream);
}

Void JxrLegacyBitReaderAdapterRefillLevel2(CWMImageStrCodec* codec, JxrLegacyBitReaderAdapter* state)
{
    readIS_L2(codec, state->stream);
}

Bool JxrLegacyBitReaderAdapterSharesStream(const JxrLegacyBitReaderAdapter* left,
    const JxrLegacyBitReaderAdapter* right)
{
    return left->stream == right->stream;
}
