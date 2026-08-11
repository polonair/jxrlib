#ifndef JXR_INDEX_TABLE_READER_H
#define JXR_INDEX_TABLE_READER_H

#include "JxrVariableLengthWordReader.h"

typedef Bool (*JxrIndexTableOperation)(Void* context);
typedef U64 (*JxrIndexTablePosition)(Void* context);
typedef Bool (*JxrIndexTableEntryConsumer)(Void* context, U32 index, U64 value);

typedef struct JxrIndexTableReader {
    JxrDecoderBitSource bitSource;
    Void* context;
    JxrIndexTableOperation refill;
    JxrIndexTableOperation alignToByte;
    JxrIndexTablePosition getPosition;
} JxrIndexTableReader;

typedef struct JxrIndexTableLegacyContext {
    CWMImageStrCodec* codec;
    BitIOInfo* input;
} JxrIndexTableLegacyContext;

Void JxrIndexTableReaderInit(JxrIndexTableReader* reader, Void* context,
    JxrDecoderBitSourceRead readBits, JxrIndexTableOperation refill,
    JxrIndexTableOperation alignToByte, JxrIndexTablePosition getPosition);
Void JxrIndexTableReaderInitLegacy(JxrIndexTableReader* reader,
    JxrIndexTableLegacyContext* legacyContext);
Bool JxrIndexTableReaderRead(JxrIndexTableReader* reader, U32 entryCount,
    JxrIndexTableEntryConsumer consumeEntry, Void* entryContext, U64* headerSize);

#endif
