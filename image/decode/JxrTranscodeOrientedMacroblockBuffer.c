#include "JxrTranscodeOrientedMacroblockBuffer.h"

static Bool JxrTranscodeOrientedMacroblockBufferHasCoefficientRange(
    size_t offset, size_t coefficientCount, size_t capacity)
{
    return offset <= capacity && coefficientCount <= capacity - offset;
}

Bool JxrTranscodeOrientedMacroblockBufferStore(
    const JxrTranscodeOrientedMacroblockBufferRequest* request)
{
    size_t macroblockOffset;
    size_t primaryCoefficientOffset;
    size_t alphaCoefficientOffset;

    if (request == NULL || request->primaryMacroblock == NULL ||
        request->primaryCoefficients == NULL || request->primaryFrameMacroblocks == NULL ||
        request->primaryFrameCoefficients == NULL || request->orientation == NULL ||
        request->destinationRow < 0 || request->destinationColumn < 0 ||
        (size_t)request->destinationRow >= request->sourceMacroblockHeight ||
        (size_t)request->destinationColumn >= request->sourceMacroblockWidth)
        return FALSE;
    macroblockOffset = JxrTranscodeOrientationStateFrameOffset(request->orientation,
        request->destinationRow, request->destinationColumn,
        request->sourceMacroblockWidth, request->sourceMacroblockHeight);
    if (macroblockOffset >= request->primaryFrameMacroblockCount ||
        request->primaryCoefficientCount > 0 &&
        macroblockOffset > ((size_t)-1) / request->primaryCoefficientCount)
        return FALSE;
    primaryCoefficientOffset = macroblockOffset * request->primaryCoefficientCount;
    if (!JxrTranscodeOrientedMacroblockBufferHasCoefficientRange(primaryCoefficientOffset,
        request->primaryCoefficientCount, request->primaryFrameCoefficientCount)) return FALSE;
    request->primaryFrameMacroblocks[macroblockOffset] = *request->primaryMacroblock;
    memcpy(request->primaryFrameCoefficients + primaryCoefficientOffset,
        request->primaryCoefficients, request->primaryCoefficientCount * sizeof(PixelI));

    if (!request->hasAlpha) return TRUE;
    if (request->alphaMacroblock == NULL || request->alphaCoefficients == NULL ||
        request->alphaFrameMacroblocks == NULL || request->alphaFrameCoefficients == NULL ||
        macroblockOffset >= request->alphaFrameMacroblockCount ||
        request->alphaCoefficientCount > 0 &&
        macroblockOffset > ((size_t)-1) / request->alphaCoefficientCount)
        return FALSE;
    alphaCoefficientOffset = macroblockOffset * request->alphaCoefficientCount;
    if (!JxrTranscodeOrientedMacroblockBufferHasCoefficientRange(alphaCoefficientOffset,
        request->alphaCoefficientCount, request->alphaFrameCoefficientCount)) return FALSE;
    request->alphaFrameMacroblocks[macroblockOffset] = *request->alphaMacroblock;
    memcpy(request->alphaFrameCoefficients + alphaCoefficientOffset,
        request->alphaCoefficients, request->alphaCoefficientCount * sizeof(PixelI));
    return TRUE;
}
