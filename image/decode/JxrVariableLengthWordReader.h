#ifndef JXR_VARIABLE_LENGTH_WORD_READER_H
#define JXR_VARIABLE_LENGTH_WORD_READER_H

#include "JxrDecoderTileQuantizerSyntaxReader.h"

Void JxrVariableLengthWordReaderInitLegacy(JxrDecoderBitSource* source,
    BitIOInfo* legacyInput);
Bool JxrVariableLengthWordReaderRead(JxrDecoderBitSource* source, U64* value,
    U8* escapeMarker);

#endif
