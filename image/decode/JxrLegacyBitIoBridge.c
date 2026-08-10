#include "JxrLegacyBitIoBridge.h"

static U8* JxrLegacyBitIoBridgeBuffer(const BitIOInfo* legacy)
{
    return (U8*)legacy - PACKETLENGTH * 2;
}

Void JxrLegacyBitIoBridgeRead(const BitIOInfo* legacy, JxrBitCursorState* cursor,
    JxrBitInputBufferState* input)
{
    U8* buffer = JxrLegacyBitIoBridgeBuffer(legacy);
    JxrBitCursorStateInit(cursor, buffer, PACKETLENGTH * 2,
        (size_t)(legacy->pbCurrent - buffer), legacy->uiAccumulator, legacy->cBitsUsed);
    JxrBitInputBufferStateInit(input, buffer, PACKETLENGTH * 2,
        (size_t)(legacy->pbStart - buffer), (size_t)(legacy->pbCurrent - buffer),
        legacy->offRef, legacy->uiShadow);
}

Void JxrLegacyBitIoBridgeApplyCursor(BitIOInfo* legacy, const JxrBitCursorState* cursor)
{
    legacy->pbCurrent = (U8*)cursor->buffer + cursor->currentIndex;
    legacy->uiAccumulator = cursor->accumulator;
    legacy->cBitsUsed = cursor->usedBits;
}

Void JxrLegacyBitIoBridgeApplyInput(BitIOInfo* legacy, const JxrBitInputBufferState* input)
{
    legacy->pbStart = input->buffer + input->packetStartIndex;
    legacy->offRef = input->streamOffset;
    legacy->uiShadow = input->shadow;
}

Bool JxrLegacyBitIoBridgeCursorMatches(const BitIOInfo* legacy, const JxrBitCursorState* cursor)
{
    return legacy->pbCurrent == cursor->buffer + cursor->currentIndex &&
        legacy->uiAccumulator == cursor->accumulator && legacy->cBitsUsed == cursor->usedBits;
}

Bool JxrLegacyBitIoBridgeInputMatches(const BitIOInfo* legacy, const JxrBitInputBufferState* input)
{
    return legacy->pbStart == input->buffer + input->packetStartIndex &&
        legacy->pbCurrent == input->buffer + input->currentIndex &&
        legacy->offRef == input->streamOffset && legacy->uiShadow == input->shadow;
}
