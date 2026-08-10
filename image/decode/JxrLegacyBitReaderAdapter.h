#ifndef JXR_LEGACY_BIT_READER_ADAPTER_H
#define JXR_LEGACY_BIT_READER_ADAPTER_H

#include "strcodec.h"

/* The only decoder-facing layer allowed to invoke legacy BitIO macros. */
typedef struct JxrLegacyBitReaderAdapter {
    BitIOInfo* stream;
} JxrLegacyBitReaderAdapter;

Void JxrLegacyBitReaderAdapterInit(JxrLegacyBitReaderAdapter* state, BitIOInfo* stream);
U32 JxrLegacyBitReaderAdapterPeek16(JxrLegacyBitReaderAdapter* state, U32 count);
Void JxrLegacyBitReaderAdapterConsume16(JxrLegacyBitReaderAdapter* state, U32 count);
U32 JxrLegacyBitReaderAdapterRead32(JxrLegacyBitReaderAdapter* state, U32 count);
Void JxrLegacyBitReaderAdapterRefillLevel1(CWMImageStrCodec* codec, JxrLegacyBitReaderAdapter* state);
Void JxrLegacyBitReaderAdapterRefillLevel2(CWMImageStrCodec* codec, JxrLegacyBitReaderAdapter* state);
Bool JxrLegacyBitReaderAdapterSharesStream(const JxrLegacyBitReaderAdapter* left,
    const JxrLegacyBitReaderAdapter* right);

#endif
