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

    symbol = JxrEntropyBlockDecoderDecodeFirstSymbol(
        context->huffmanStates[coefficientContextBase], context->highpassInput);
    significantRun = symbol & 1;
    remainingSignificantRuns = symbol >> 2;
    continuation = significantRun & remainingSignificantRuns;
    sign = JxrEntropyReaderReadSign(context->highpassInput);
    level = JxrHpBlockDecoderApplySign(scaledQuantizationParameter, sign);
    if (symbol & 2) {
        Int magnitude = JxrEntropyLevelDecoderDecode(
            context->huffmanStates[6 + CTDC + CONTEXTX + continuation], context->highpassInput);
        level *= magnitude;
    }
    if (significantRun == 0) {
        location += JxrEntropyBlockDecoderDecodeRun(15 - location,
            context->huffmanStates[0], context->highpassInput);
    }
    location &= 15;
    JxrCoefficientBufferSet(context->coefficientBuffer,
        JxrAdaptiveScanGetCoefficientIndex(context->scan, location), (PixelI)level);
    JxrAdaptiveScanObserveNonZero(context->scan, location);
    location = (location + 1) & 15;

    while (remainingSignificantRuns != 0) {
        significantRun = remainingSignificantRuns & 1;
        if (significantRun == 0) {
            location += JxrEntropyBlockDecoderDecodeRun(15 - location,
                context->huffmanStates[0], context->highpassInput);
            if (location >= 16)
                return 16;
        }
        symbol = JxrEntropyBlockDecoderDecodeNextSymbol(location + 1,
            context->huffmanStates[coefficientContextBase + continuation + 1], context->highpassInput);
        remainingSignificantRuns = symbol >> 1;

        assert(remainingSignificantRuns >= 0 && remainingSignificantRuns < 3);
        continuation &= remainingSignificantRuns;
        sign = JxrEntropyReaderReadSign(context->highpassInput);
        level = JxrHpBlockDecoderApplySign(scaledQuantizationParameter, sign);
        if (symbol & 1) {
            Int magnitude = JxrEntropyLevelDecoderDecode(
                context->huffmanStates[6 + CTDC + CONTEXTX + continuation], context->highpassInput);
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
                Int fine = JxrEntropyReaderRead(context->flexbitsInput, flexbitCount);
                JxrCoefficientBufferAdd(context->coefficientBuffer, coefficientIndex, (PixelI)(-fine));
            }
            else if (coefficient > 0) {
                Int fine = JxrEntropyReaderRead(context->flexbitsInput, flexbitCount);
                JxrCoefficientBufferAdd(context->coefficientBuffer, coefficientIndex, (PixelI)fine);
            }
            else {
                JxrCoefficientBufferSet(context->coefficientBuffer, coefficientIndex,
                    (PixelI)JxrEntropyReaderReadSignedResidual(context->flexbitsInput, flexbitCount));
            }
        }
    }
    else {
        const Int flexbitQuantizationParameter = context->quantizationParameter << context->trimFlexBits;
        for (index = 1; index < 16; ++index) {
            Int coefficientIndex = context->coefficientOrder[index];
            Int coefficient = JxrCoefficientBufferGet(context->coefficientBuffer, coefficientIndex);
            if (coefficient < 0) {
                Int fine = JxrEntropyReaderRead(context->flexbitsInput, flexbitCount);
                JxrCoefficientBufferAdd(context->coefficientBuffer, coefficientIndex,
                    (PixelI)(-flexbitQuantizationParameter * fine));
            }
            else if (coefficient > 0) {
                Int fine = JxrEntropyReaderRead(context->flexbitsInput, flexbitCount);
                JxrCoefficientBufferAdd(context->coefficientBuffer, coefficientIndex,
                    (PixelI)(flexbitQuantizationParameter * fine));
            }
            else {
                JxrCoefficientBufferSet(context->coefficientBuffer, coefficientIndex,
                    (PixelI)(flexbitQuantizationParameter *
                        JxrEntropyReaderReadSignedResidual(context->flexbitsInput, flexbitCount)));
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
