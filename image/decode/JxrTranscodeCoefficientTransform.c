#include "JxrTranscodeCoefficientTransform.h"

#include "strcodec.h"

#define JXR_TRANSCODE_DC_COEFFICIENT_COUNT 16
#define JXR_TRANSCODE_AC_BLOCK_COUNT 16
#define JXR_TRANSCODE_AC_COEFFICIENT_COUNT 16
#define JXR_TRANSCODE_AC_TOTAL_COUNT (JXR_TRANSCODE_AC_BLOCK_COUNT * JXR_TRANSCODE_AC_COEFFICIENT_COUNT)

static Bool JxrTranscodeCoefficientBufferHasRange(const JxrTranscodeCoefficientBuffer* buffer,
    size_t requiredCount)
{
    return buffer != NULL && buffer->values != NULL && buffer->offset <= buffer->count &&
        requiredCount <= buffer->count - buffer->offset;
}

Void JxrTranscodeCoefficientBufferInit(JxrTranscodeCoefficientBuffer* buffer,
    PixelI* values, size_t offset, size_t count)
{
    buffer->values = values;
    buffer->offset = offset;
    buffer->count = count;
}

Bool JxrTranscodeCoefficientTransformDc444(JxrTranscodeCoefficientBuffer* source,
    JxrTranscodeCoefficientBuffer* destination,
    const JxrTranscodeOrientationState* orientation)
{
    PixelI* sourceValues;
    PixelI* destinationValues;
    size_t index;

    if (!JxrTranscodeCoefficientBufferHasRange(source, JXR_TRANSCODE_DC_COEFFICIENT_COUNT) ||
        !JxrTranscodeCoefficientBufferHasRange(destination, JXR_TRANSCODE_DC_COEFFICIENT_COUNT) ||
        orientation == NULL) return FALSE;
    sourceValues = source->values + source->offset;
    destinationValues = destination->values + destination->offset;

    if (orientation->flipVertical)
        for (index = 0; index < JXR_TRANSCODE_DC_COEFFICIENT_COUNT; index += 4) {
            sourceValues[index + 1] = -sourceValues[index + 1];
            sourceValues[index + 3] = -sourceValues[index + 3];
        }
    if (orientation->flipHorizontal)
        for (index = 0; index < 4; ++index) {
            sourceValues[index + 4] = -sourceValues[index + 4];
            sourceValues[index + 12] = -sourceValues[index + 12];
        }
    if (!orientation->transpose)
        memcpy(destinationValues, sourceValues, JXR_TRANSCODE_DC_COEFFICIENT_COUNT * sizeof(PixelI));
    else
        for (index = 0; index < JXR_TRANSCODE_DC_COEFFICIENT_COUNT; ++index)
            destinationValues[index] = sourceValues[(index >> 2) + ((index & 3) << 2)];
    return TRUE;
}

Bool JxrTranscodeCoefficientTransformAc444(JxrTranscodeCoefficientBuffer* source,
    JxrTranscodeCoefficientBuffer* destination,
    const JxrTranscodeOrientationState* orientation)
{
    PixelI* sourceValues;
    PixelI* destinationValues;
    PixelI* sourceBlock;
    PixelI* destinationBlock;
    const Int* transformIndex = dctIndex[0];
    size_t blockRow;
    size_t blockColumn;
    size_t coefficient;

    if (!JxrTranscodeCoefficientBufferHasRange(source, JXR_TRANSCODE_AC_TOTAL_COUNT) ||
        !JxrTranscodeCoefficientBufferHasRange(destination, JXR_TRANSCODE_AC_TOTAL_COUNT) ||
        orientation == NULL) return FALSE;
    sourceValues = source->values + source->offset;
    destinationValues = destination->values + destination->offset;

    for (blockRow = 0, sourceBlock = sourceValues; blockRow < JXR_TRANSCODE_AC_BLOCK_COUNT;
        ++blockRow, sourceBlock += JXR_TRANSCODE_AC_COEFFICIENT_COUNT) {
        if (orientation->flipVertical)
            for (coefficient = 0; coefficient < JXR_TRANSCODE_AC_COEFFICIENT_COUNT;
                coefficient += 4) {
                sourceBlock[transformIndex[coefficient + 1]] = -sourceBlock[transformIndex[coefficient + 1]];
                sourceBlock[transformIndex[coefficient + 3]] = -sourceBlock[transformIndex[coefficient + 3]];
            }
        if (orientation->flipHorizontal)
            for (coefficient = 0; coefficient < 4; ++coefficient) {
                sourceBlock[transformIndex[coefficient + 4]] = -sourceBlock[transformIndex[coefficient + 4]];
                sourceBlock[transformIndex[coefficient + 12]] = -sourceBlock[transformIndex[coefficient + 12]];
            }
    }

    for (blockRow = 0; blockRow < 4; ++blockRow)
        for (blockColumn = 0; blockColumn < 4; ++blockColumn) {
            size_t destinationRow = orientation->flipVertical ? 3 - blockColumn : blockColumn;
            size_t destinationColumn = orientation->flipHorizontal ? 3 - blockRow : blockRow;
            sourceBlock = sourceValues + (blockRow * 4 + blockColumn) * JXR_TRANSCODE_AC_COEFFICIENT_COUNT;
            if (!orientation->transpose)
                memcpy(destinationValues + (destinationColumn * 4 + destinationRow) *
                    JXR_TRANSCODE_AC_COEFFICIENT_COUNT, sourceBlock,
                    JXR_TRANSCODE_AC_COEFFICIENT_COUNT * sizeof(PixelI));
            else {
                destinationBlock = destinationValues + (destinationRow * 4 + destinationColumn) *
                    JXR_TRANSCODE_AC_COEFFICIENT_COUNT;
                for (coefficient = 1; coefficient < JXR_TRANSCODE_AC_COEFFICIENT_COUNT;
                    ++coefficient)
                    destinationBlock[transformIndex[coefficient]] =
                        sourceBlock[transformIndex[(coefficient >> 2) + ((coefficient & 3) << 2)]];
            }
        }
    return TRUE;
}

Bool JxrTranscodeCoefficientTransformDc422(JxrTranscodeCoefficientBuffer* source,
    JxrTranscodeCoefficientBuffer* destination,
    const JxrTranscodeOrientationState* orientation)
{
    PixelI* sourceValues;
    PixelI* destinationValues;

    if (!JxrTranscodeCoefficientBufferHasRange(source, 8) ||
        !JxrTranscodeCoefficientBufferHasRange(destination, 8) || orientation == NULL ||
        orientation->transpose) return FALSE;
    sourceValues = source->values + source->offset;
    destinationValues = destination->values + destination->offset;
    if (orientation->flipVertical) {
        sourceValues[1] = -sourceValues[1]; sourceValues[3] = -sourceValues[3];
        sourceValues[4] = -sourceValues[4]; sourceValues[5] = -sourceValues[5];
        sourceValues[7] = -sourceValues[7];
    }
    if (orientation->flipHorizontal) {
        sourceValues[2] = -sourceValues[2]; sourceValues[3] = -sourceValues[3];
        sourceValues[6] = -sourceValues[6]; sourceValues[7] = -sourceValues[7];
    }
    if (orientation->flipVertical) {
        destinationValues[0] = sourceValues[0]; destinationValues[1] = sourceValues[5];
        destinationValues[2] = sourceValues[6]; destinationValues[3] = sourceValues[7];
        destinationValues[4] = sourceValues[4]; destinationValues[5] = sourceValues[1];
        destinationValues[6] = sourceValues[2]; destinationValues[7] = sourceValues[3];
    }
    else memcpy(destinationValues, sourceValues, 8 * sizeof(PixelI));
    return TRUE;
}

Bool JxrTranscodeCoefficientTransformAc422(JxrTranscodeCoefficientBuffer* source,
    JxrTranscodeCoefficientBuffer* destination,
    const JxrTranscodeOrientationState* orientation)
{
    PixelI* sourceValues;
    PixelI* destinationValues;
    PixelI* sourceBlock;
    const Int* transformIndex = dctIndex[0];
    size_t blockRow;
    size_t blockColumn;
    size_t coefficient;

    if (!JxrTranscodeCoefficientBufferHasRange(source, 128) ||
        !JxrTranscodeCoefficientBufferHasRange(destination, 128) || orientation == NULL ||
        orientation->transpose) return FALSE;
    sourceValues = source->values + source->offset;
    destinationValues = destination->values + destination->offset;
    for (blockRow = 0, sourceBlock = sourceValues; blockRow < 8; ++blockRow,
        sourceBlock += JXR_TRANSCODE_AC_COEFFICIENT_COUNT) {
        if (orientation->flipVertical)
            for (coefficient = 0; coefficient < JXR_TRANSCODE_AC_COEFFICIENT_COUNT;
                coefficient += 4) {
                sourceBlock[transformIndex[coefficient + 1]] = -sourceBlock[transformIndex[coefficient + 1]];
                sourceBlock[transformIndex[coefficient + 3]] = -sourceBlock[transformIndex[coefficient + 3]];
            }
        if (orientation->flipHorizontal)
            for (coefficient = 0; coefficient < 4; ++coefficient) {
                sourceBlock[transformIndex[coefficient + 4]] = -sourceBlock[transformIndex[coefficient + 4]];
                sourceBlock[transformIndex[coefficient + 12]] = -sourceBlock[transformIndex[coefficient + 12]];
            }
    }
    for (blockRow = 0; blockRow < 2; ++blockRow)
        for (blockColumn = 0; blockColumn < 4; ++blockColumn) {
            size_t destinationRow = orientation->flipVertical ? 3 - blockColumn : blockColumn;
            size_t destinationColumn = orientation->flipHorizontal ? 1 - blockRow : blockRow;
            memcpy(destinationValues + (destinationColumn * 4 + destinationRow) *
                JXR_TRANSCODE_AC_COEFFICIENT_COUNT,
                sourceValues + (blockRow * 4 + blockColumn) * JXR_TRANSCODE_AC_COEFFICIENT_COUNT,
                JXR_TRANSCODE_AC_COEFFICIENT_COUNT * sizeof(PixelI));
        }
    return TRUE;
}

Bool JxrTranscodeCoefficientTransformDc420(JxrTranscodeCoefficientBuffer* source,
    JxrTranscodeCoefficientBuffer* destination,
    const JxrTranscodeOrientationState* orientation)
{
    PixelI* sourceValues;
    PixelI* destinationValues;

    if (!JxrTranscodeCoefficientBufferHasRange(source, 4) ||
        !JxrTranscodeCoefficientBufferHasRange(destination, 4) || orientation == NULL)
        return FALSE;
    sourceValues = source->values + source->offset;
    destinationValues = destination->values + destination->offset;
    if (orientation->flipVertical) {
        sourceValues[1] = -sourceValues[1];
        sourceValues[3] = -sourceValues[3];
    }
    if (orientation->flipHorizontal) {
        sourceValues[2] = -sourceValues[2];
        sourceValues[3] = -sourceValues[3];
    }
    destinationValues[0] = sourceValues[0];
    destinationValues[3] = sourceValues[3];
    if (!orientation->transpose) {
        destinationValues[1] = sourceValues[1];
        destinationValues[2] = sourceValues[2];
    }
    else {
        destinationValues[1] = sourceValues[2];
        destinationValues[2] = sourceValues[1];
    }
    return TRUE;
}

Bool JxrTranscodeCoefficientTransformAc420(JxrTranscodeCoefficientBuffer* source,
    JxrTranscodeCoefficientBuffer* destination,
    const JxrTranscodeOrientationState* orientation)
{
    PixelI* sourceValues;
    PixelI* destinationValues;
    PixelI* sourceBlock;
    PixelI* destinationBlock;
    const Int* transformIndex = dctIndex[0];
    size_t blockRow;
    size_t blockColumn;
    size_t coefficient;

    if (!JxrTranscodeCoefficientBufferHasRange(source, 64) ||
        !JxrTranscodeCoefficientBufferHasRange(destination, 64) || orientation == NULL)
        return FALSE;
    sourceValues = source->values + source->offset;
    destinationValues = destination->values + destination->offset;
    for (blockRow = 0, sourceBlock = sourceValues; blockRow < 4; ++blockRow,
        sourceBlock += JXR_TRANSCODE_AC_COEFFICIENT_COUNT) {
        if (orientation->flipVertical)
            for (coefficient = 0; coefficient < JXR_TRANSCODE_AC_COEFFICIENT_COUNT;
                coefficient += 4) {
                sourceBlock[transformIndex[coefficient + 1]] = -sourceBlock[transformIndex[coefficient + 1]];
                sourceBlock[transformIndex[coefficient + 3]] = -sourceBlock[transformIndex[coefficient + 3]];
            }
        if (orientation->flipHorizontal)
            for (coefficient = 0; coefficient < 4; ++coefficient) {
                sourceBlock[transformIndex[coefficient + 4]] = -sourceBlock[transformIndex[coefficient + 4]];
                sourceBlock[transformIndex[coefficient + 12]] = -sourceBlock[transformIndex[coefficient + 12]];
            }
    }
    for (blockRow = 0; blockRow < 2; ++blockRow)
        for (blockColumn = 0; blockColumn < 2; ++blockColumn) {
            size_t destinationRow = orientation->flipVertical ? 1 - blockColumn : blockColumn;
            size_t destinationColumn = orientation->flipHorizontal ? 1 - blockRow : blockRow;
            sourceBlock = sourceValues + (blockRow * 2 + blockColumn) * JXR_TRANSCODE_AC_COEFFICIENT_COUNT;
            if (!orientation->transpose)
                memcpy(destinationValues + (destinationColumn * 2 + destinationRow) *
                    JXR_TRANSCODE_AC_COEFFICIENT_COUNT, sourceBlock,
                    JXR_TRANSCODE_AC_COEFFICIENT_COUNT * sizeof(PixelI));
            else {
                destinationBlock = destinationValues + (destinationRow * 2 + destinationColumn) *
                    JXR_TRANSCODE_AC_COEFFICIENT_COUNT;
                for (coefficient = 1; coefficient < JXR_TRANSCODE_AC_COEFFICIENT_COUNT;
                    ++coefficient)
                    destinationBlock[transformIndex[coefficient]] =
                        sourceBlock[transformIndex[(coefficient >> 2) + ((coefficient & 3) << 2)]];
            }
        }
    return TRUE;
}
