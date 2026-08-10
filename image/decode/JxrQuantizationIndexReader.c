#include "JxrQuantizationIndexReader.h"

U8 JxrQuantizationIndexReaderDecode(JxrEntropyBitReader* reader, U8 bitCount)
{
    if (JxrEntropyBitReaderReadFlag(reader) == 0)
        return 0;
    return (U8)(JxrEntropyBitReaderRead(reader, bitCount) + 1);
}
