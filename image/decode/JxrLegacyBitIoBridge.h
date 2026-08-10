#ifndef JXR_LEGACY_BIT_IO_BRIDGE_H
#define JXR_LEGACY_BIT_IO_BRIDGE_H

#include "strcodec.h"
#include "JxrBitCursorState.h"
#include "JxrBitInputBufferState.h"

/* The only mapping layer between native BitIOInfo and index-based decoder state. */
Void JxrLegacyBitIoBridgeRead(const BitIOInfo* legacy, JxrBitCursorState* cursor,
    JxrBitInputBufferState* input);
Void JxrLegacyBitIoBridgeApplyCursor(BitIOInfo* legacy, const JxrBitCursorState* cursor);
Void JxrLegacyBitIoBridgeApplyInput(BitIOInfo* legacy, const JxrBitInputBufferState* input);
Bool JxrLegacyBitIoBridgeCursorMatches(const BitIOInfo* legacy, const JxrBitCursorState* cursor);
Bool JxrLegacyBitIoBridgeInputMatches(const BitIOInfo* legacy, const JxrBitInputBufferState* input);

#endif
