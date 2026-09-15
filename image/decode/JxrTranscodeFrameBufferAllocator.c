#include "JxrTranscodeFrameBufferAllocator.h"

static Bool JxrTranscodeFrameBufferAllocatorMultiply(size_t left, size_t right,
    size_t* result)
{
    if (result == NULL || (right != 0 && left > ((size_t)-1) / right))
        return FALSE;
    *result = left * right;
    return TRUE;
}

static Bool JxrTranscodeFrameBufferAllocatorByteCount(size_t elementCount,
    size_t elementSize, size_t* byteCount)
{
    return JxrTranscodeFrameBufferAllocatorMultiply(elementCount, elementSize,
        byteCount);
}

Void JxrTranscodeFrameBufferAllocatorRelease(
    JxrTranscodeFrameBufferAllocation* allocation)
{
    if (allocation == NULL) return;
    free(allocation->primaryCoefficients);
    free(allocation->alphaCoefficients);
    free(allocation->primaryMacroblocks);
    free(allocation->alphaMacroblocks);
    memset(allocation, 0, sizeof(*allocation));
}

Int JxrTranscodeFrameBufferAllocatorAllocate(CWMImageStrCodec* encoderCodec,
    const CWMTranscodingParam* parameters, ORIENTATION orientation,
    size_t coefficientUnit, JxrTranscodeFrameBufferAllocation* allocation)
{
    size_t macroblockCount;
    size_t primaryCoefficientCount;
    size_t alphaCoefficientCount;
    size_t primaryCoefficientBytes;
    size_t alphaCoefficientBytes;
    size_t macroblockBytes;

    if (encoderCodec == NULL || parameters == NULL || allocation == NULL)
        return ICERR_ERROR;
    memset(allocation, 0, sizeof(*allocation));
    if (orientation == O_NONE) return ICERR_OK;
    if (!JxrTranscodeFrameBufferAllocatorMultiply(encoderCodec->cmbWidth,
        encoderCodec->cmbHeight, &macroblockCount) ||
        !JxrTranscodeFrameBufferAllocatorMultiply(macroblockCount,
            coefficientUnit, &primaryCoefficientCount) ||
        !JxrTranscodeFrameBufferAllocatorByteCount(primaryCoefficientCount,
            sizeof(PixelI), &primaryCoefficientBytes) ||
        !JxrTranscodeFrameBufferAllocatorByteCount(macroblockCount,
            sizeof(CWMIMBInfo), &macroblockBytes))
        return ICERR_ERROR;

    allocation->primaryCoefficients = (PixelI*)malloc(primaryCoefficientBytes);
    allocation->primaryMacroblocks = (CWMIMBInfo*)malloc(macroblockBytes);
    if (allocation->primaryCoefficients == NULL ||
        allocation->primaryMacroblocks == NULL) {
        JxrTranscodeFrameBufferAllocatorRelease(allocation);
        return ICERR_ERROR;
    }
    if (parameters->uAlphaMode == 0) return ICERR_OK;
    if (!JxrTranscodeFrameBufferAllocatorMultiply(macroblockCount, 256,
        &alphaCoefficientCount) ||
        !JxrTranscodeFrameBufferAllocatorByteCount(alphaCoefficientCount,
            sizeof(PixelI), &alphaCoefficientBytes)) {
        JxrTranscodeFrameBufferAllocatorRelease(allocation);
        return ICERR_ERROR;
    }

    allocation->alphaCoefficients = (PixelI*)malloc(alphaCoefficientBytes);
    allocation->alphaMacroblocks = (CWMIMBInfo*)malloc(macroblockBytes);
    if (allocation->alphaCoefficients == NULL || allocation->alphaMacroblocks == NULL) {
        JxrTranscodeFrameBufferAllocatorRelease(allocation);
        return ICERR_ERROR;
    }
    return ICERR_OK;
}
