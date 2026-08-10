#ifndef JXR_QUANTIZATION_INDEX_READER_H
#define JXR_QUANTIZATION_INDEX_READER_H

#include "JxrEntropyReader.h"

U8 JxrQuantizationIndexReaderDecode(JxrEntropyBitReader* reader, U8 bitCount);

#endif
