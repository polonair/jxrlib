#include "JxrEntropyBlockDecoder.h"
#include "JxrAdaptiveHuffman.h"
#include "JxrEntropyLevelDecoder.h"
#include "JxrEntropyReader.h"

Int JxrEntropyBlockDecoderDecodeRun(Int maximumRun, CAdaptiveHuffman* state, BitIOInfo* input)
{
    static const Int remap[] = { 1,2,3,5,7, 1,2,3,5,7, 1,2,3,4,5 };
    Int tableIndex;
    Int fixedBitCount;
    Int run;

    if (maximumRun < 5) {
        if (maximumRun == 1 || JxrEntropyReaderReadFlag(input)) return 1;
        if (maximumRun == 2 || JxrEntropyReaderReadFlag(input)) return 2;
        if (maximumRun == 3 || JxrEntropyReaderReadFlag(input)) return 3;
        return 4;
    }

    tableIndex = JxrAdaptiveHuffmanDecodeShortTable(state->m_hufDecTable, input);
    tableIndex += gSignificantRunBin[maximumRun] * 5;
    run = remap[tableIndex];
    fixedBitCount = gSignificantRunFixedLength[tableIndex];
    if (fixedBitCount) run += (Int)JxrEntropyReaderRead(input, fixedBitCount);
    return run;
}

Int JxrEntropyBlockDecoderDecodeFirstSymbol(CAdaptiveHuffman* state, BitIOInfo* input)
{
    return JxrAdaptiveHuffmanDecode(state, input);
}

Int JxrEntropyBlockDecoderDecodeNextSymbol(Int coefficientPosition,
    CAdaptiveHuffman* state, BitIOInfo* input)
{
    Int symbol;
    if (coefficientPosition < 15) {
        symbol = JxrAdaptiveHuffmanDecodeShortTable(state->m_hufDecTable, input);
        JxrAdaptiveHuffmanObserve(state, symbol);
        return symbol;
    }
    if (coefficientPosition == 15) {
        if (!JxrEntropyReaderReadFlag(input)) return 0;
        if (!JxrEntropyReaderReadFlag(input)) return 2;
        return 1 + 2 * (Int)JxrEntropyReaderReadFlag(input);
    }
    return (Int)JxrEntropyReaderRead(input, 1);
}

Int JxrEntropyBlockDecoderDecodeLowpassBlock(Bool isChroma, Int* runLevelPairs,
    CAdaptiveHuffman** huffmanStates, Int contextOffset, BitIOInfo* input, Int startPosition)
{
    Int significantRun;
    Int remainingSymbols;
    Int symbol;
    Int nonZeroCount = 1;
    Int context;
    Int sign;
    CAdaptiveHuffman** symbolStates = huffmanStates + contextOffset + isChroma * 3;

    symbol = JxrEntropyBlockDecoderDecodeFirstSymbol(symbolStates[0], input);
    significantRun = symbol & 1;
    remainingSymbols = symbol >> 2;
    context = significantRun & remainingSymbols;
    sign = JxrEntropyReaderReadSign(input);
    if (symbol & 2) {
        runLevelPairs[1] = (JxrEntropyLevelDecoderDecode(huffmanStates[6 + contextOffset + context], input) ^ sign) - sign;
    }
    else {
        runLevelPairs[1] = 1 | sign;
    }

    runLevelPairs[0] = 0;
    if (!significantRun) runLevelPairs[0] = JxrEntropyBlockDecoderDecodeRun(15 - startPosition, huffmanStates[0], input);
    startPosition += runLevelPairs[0] + 1;

    while (remainingSymbols) {
        significantRun = remainingSymbols & 1;
        runLevelPairs[nonZeroCount * 2] = 0;
        if (!significantRun) {
            runLevelPairs[nonZeroCount * 2] = JxrEntropyBlockDecoderDecodeRun(15 - startPosition, huffmanStates[0], input);
        }
        startPosition += runLevelPairs[nonZeroCount * 2] + 1;
        symbol = JxrEntropyBlockDecoderDecodeNextSymbol(startPosition, symbolStates[context + 1], input);
        remainingSymbols = symbol >> 1;
        assert(remainingSymbols >= 0 && remainingSymbols < 3);
        context &= remainingSymbols;
        sign = JxrEntropyReaderReadSign(input);
        if (symbol & 1) {
            runLevelPairs[nonZeroCount * 2 + 1] =
                (JxrEntropyLevelDecoderDecode(huffmanStates[6 + contextOffset + context], input) ^ sign) - sign;
        }
        else {
            runLevelPairs[nonZeroCount * 2 + 1] = 1 | sign;
        }
        nonZeroCount++;
    }
    return nonZeroCount;
}
