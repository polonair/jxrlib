#include "JxrEntropyLevelDecoder.h"
#include "JxrAdaptiveHuffman.h"
#include "JxrEntropyReader.h"

Int JxrEntropyLevelDecoderDecodeReader(CAdaptiveHuffman* state, JxrEntropyBitReader* input)
{
    UInt symbol;
    Int extraBitCount;
    Int level;
    static const Int baseLevels[] = { 2, 3, 4, 6, 10, 14 };
    static const Int extraBitCounts[] = { 0, 0, 1, 2, 2, 2 };

    symbol = (UInt)JxrAdaptiveHuffmanDecodeReader(state, input);
    assert(symbol <= 6);
    if (symbol < 2) {
        return (Int)symbol + 2;
    }
    if (symbol < 6) {
        extraBitCount = extraBitCounts[symbol];
        return baseLevels[symbol] + (Int)JxrEntropyBitReaderRead(input, extraBitCount);
    }

    extraBitCount = (Int)JxrEntropyBitReaderRead(input, 4) + 4;
    if (extraBitCount == 19) {
        extraBitCount += (Int)JxrEntropyBitReaderRead(input, 2);
        if (extraBitCount == 22) {
            extraBitCount += (Int)JxrEntropyBitReaderRead(input, 3);
        }
    }
    level = 2 + (1 << extraBitCount);
    return level + (Int)JxrEntropyBitReaderReadLong(input, extraBitCount);
}

Int JxrEntropyLevelDecoderDecode(CAdaptiveHuffman* state, BitIOInfo* input)
{
    JxrEntropyBitReader reader;
    JxrEntropyBitReaderInit(&reader, input);
    return JxrEntropyLevelDecoderDecodeReader(state, &reader);
}
