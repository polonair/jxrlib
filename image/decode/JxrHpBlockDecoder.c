#include "JxrHpBlockDecoder.h"
#include "JxrEntropyBlockDecoder.h"
#include "JxrEntropyLevelDecoder.h"
#include "JxrEntropyReader.h"

static Int JxrHpBlockDecoderApplySign(Int magnitude, I32 sign)
{
    return sign ? -magnitude : magnitude;
}

static Int JxrHpBlockDecoderDecodeEntropyCoefficients(JxrHpBlockDecodingContext* context,
    Int scaledQuantizationParameter)
{
    const Int coefficientContextBase = CTDC + CONTEXTX + (context->isChroma ? 3 : 0);
    UInt location = 1;
    Int significantRun;
    Int remainingSignificantRuns;
    Int symbol;
    Int nonzeroCount = 1;
    Int continuation;
    I32 sign;
    Int level;

    symbol = JxrEntropyBlockDecoderDecodeFirstSymbolReader(
        JxrHuffmanStateSetGet(context->huffmanStateSet, coefficientContextBase), context->highpassReader);
    significantRun = symbol & 1;
    remainingSignificantRuns = symbol >> 2;
    continuation = significantRun & remainingSignificantRuns;
    sign = JxrEntropyBitReaderReadSign(context->highpassReader);
    level = JxrHpBlockDecoderApplySign(scaledQuantizationParameter, sign);
    if (symbol & 2) {
        Int magnitude = JxrEntropyLevelDecoderDecodeReader(
            JxrHuffmanStateSetGet(context->huffmanStateSet, 6 + CTDC + CONTEXTX + continuation), context->highpassReader);
        level *= magnitude;
    }
    if (significantRun == 0) {
        location += JxrEntropyBlockDecoderDecodeRunReader(15 - location,
            JxrHuffmanStateSetGet(context->huffmanStateSet, 0), context->highpassReader);
    }
    location &= 15;
    JxrCoefficientBufferSet(context->coefficientBuffer,
        JxrAdaptiveScanGetCoefficientIndex(context->scan, location), (PixelI)level);
    JxrAdaptiveScanObserveNonZero(context->scan, location);
    location = (location + 1) & 15;

    while (remainingSignificantRuns != 0) {
        significantRun = remainingSignificantRuns & 1;
        if (significantRun == 0) {
            location += JxrEntropyBlockDecoderDecodeRunReader(15 - location,
                JxrHuffmanStateSetGet(context->huffmanStateSet, 0), context->highpassReader);
            if (location >= 16)
                return 16;
        }
        symbol = JxrEntropyBlockDecoderDecodeNextSymbolReader(location + 1,
            JxrHuffmanStateSetGet(context->huffmanStateSet, coefficientContextBase + continuation + 1), context->highpassReader);
        remainingSignificantRuns = symbol >> 1;

        assert(remainingSignificantRuns >= 0 && remainingSignificantRuns < 3);
        continuation &= remainingSignificantRuns;
        sign = JxrEntropyBitReaderReadSign(context->highpassReader);
        level = JxrHpBlockDecoderApplySign(scaledQuantizationParameter, sign);
        if (symbol & 1) {
            Int magnitude = JxrEntropyLevelDecoderDecodeReader(
                JxrHuffmanStateSetGet(context->huffmanStateSet, 6 + CTDC + CONTEXTX + continuation), context->highpassReader);
            level *= magnitude;
        }
        JxrCoefficientBufferSet(context->coefficientBuffer,
            JxrAdaptiveScanGetCoefficientIndex(context->scan, location), (PixelI)level);
        JxrAdaptiveScanObserveNonZero(context->scan, location);
        location = (location + 1) & 15;
        nonzeroCount++;
    }
    return nonzeroCount;
}

static Void JxrHpBlockDecoderDecodeFlexbits(JxrHpBlockDecodingContext* context, Int flexbitCount)
{
    Int index;
    if (context->quantizationParameter + context->trimFlexBits == 1) {
        assert(context->trimFlexBits == 0);
        assert(context->quantizationParameter == 1);
        for (index = 1; index < 16; ++index) {
            Int coefficientIndex = context->coefficientOrder[index];
            PixelI coefficient = JxrCoefficientBufferGet(context->coefficientBuffer, coefficientIndex);
            if (coefficient < 0) {
                Int fine = JxrEntropyBitReaderRead(context->flexbitsReader, flexbitCount);
                JxrCoefficientBufferAdd(context->coefficientBuffer, coefficientIndex, (PixelI)(-fine));
            }
            else if (coefficient > 0) {
                Int fine = JxrEntropyBitReaderRead(context->flexbitsReader, flexbitCount);
                JxrCoefficientBufferAdd(context->coefficientBuffer, coefficientIndex, (PixelI)fine);
            }
            else {
                JxrCoefficientBufferSet(context->coefficientBuffer, coefficientIndex,
                    (PixelI)JxrEntropyBitReaderReadSignedResidual(context->flexbitsReader, flexbitCount));
            }
        }
    }
    else {
        const Int flexbitQuantizationParameter = context->quantizationParameter << context->trimFlexBits;
        for (index = 1; index < 16; ++index) {
            Int coefficientIndex = context->coefficientOrder[index];
            Int coefficient = JxrCoefficientBufferGet(context->coefficientBuffer, coefficientIndex);
            if (coefficient < 0) {
                Int fine = JxrEntropyBitReaderRead(context->flexbitsReader, flexbitCount);
                JxrCoefficientBufferAdd(context->coefficientBuffer, coefficientIndex,
                    (PixelI)(-flexbitQuantizationParameter * fine));
            }
            else if (coefficient > 0) {
                Int fine = JxrEntropyBitReaderRead(context->flexbitsReader, flexbitCount);
                JxrCoefficientBufferAdd(context->coefficientBuffer, coefficientIndex,
                    (PixelI)(flexbitQuantizationParameter * fine));
            }
            else {
                JxrCoefficientBufferSet(context->coefficientBuffer, coefficientIndex,
                    (PixelI)(flexbitQuantizationParameter *
                        JxrEntropyBitReaderReadSignedResidual(context->flexbitsReader, flexbitCount)));
            }
        }
    }
}

Int JxrHpBlockDecoderDecode(JxrHpBlockDecodingContext* context)
{
    Int flexbitCount = context->modelBits - context->trimFlexBits;
    Int nonzeroCount = 0;

    if (flexbitCount < 0 || context->skipFlexbits)
        flexbitCount = 0;
    if (context->hasCoefficients) {
        const Int scaledQuantizationParameter = context->quantizationParameter << context->modelBits;
        nonzeroCount = JxrHpBlockDecoderDecodeEntropyCoefficients(context,
            scaledQuantizationParameter);
    }
    if (flexbitCount)
        JxrHpBlockDecoderDecodeFlexbits(context, flexbitCount);

    return nonzeroCount;
}
