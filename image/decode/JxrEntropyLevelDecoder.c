#include "JxrEntropyLevelDecoder.h"
#include "JxrAdaptiveHuffman.h"
#include "JxrEntropyReader.h"

Int JxrEntropyLevelDecoderDecode(CAdaptiveHuffman* state, BitIOInfo* input)
{
    UInt symbol;
    Int extraBitCount;
    Int level;
    static const Int baseLevels[] = { 2, 3, 4, 6, 10, 14 };
    static const Int extraBitCounts[] = { 0, 0, 1, 2, 2, 2 };

    symbol = (UInt)JxrAdaptiveHuffmanDecode(state, input);
    assert(symbol <= 6);
    if (symbol < 2) {
        return (Int)symbol + 2;
    }
    if (symbol < 6) {
        extraBitCount = extraBitCounts[symbol];
        return baseLevels[symbol] + (Int)JxrEntropyReaderRead(input, extraBitCount);
    }

    extraBitCount = (Int)JxrEntropyReaderRead(input, 4) + 4;
    if (extraBitCount == 19) {
        extraBitCount += (Int)JxrEntropyReaderRead(input, 2);
        if (extraBitCount == 22) {
            extraBitCount += (Int)JxrEntropyReaderRead(input, 3);
        }
    }
    level = 2 + (1 << extraBitCount);
    return level + (Int)getBit32(input, extraBitCount);
}
