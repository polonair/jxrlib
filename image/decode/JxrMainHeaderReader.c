#include "JxrMainHeaderReader.h"
#include <string.h>

static Bool read_bits(SimpleBitIO* input, U32 count, U32* value)
{
    if (input == NULL || value == NULL) return FALSE;
    *value = getBit32_SB(input, count);
    return TRUE;
}

static Bool read_tile_sizes(SimpleBitIO* input, Bool abbreviated, U32 count, U32 values[MAX_TILES])
{
    U32 index, value;
    values[0] = 0;
    for (index = 0; index < count; ++index) {
        if (!read_bits(input, abbreviated ? 8 : 16, &value)) return FALSE;
        values[index + 1] = values[index] + value;
    }
    return TRUE;
}

Bool JxrMainHeaderReaderRead(SimpleBitIO* input, JxrMainHeaderDescriptor* result)
{
    U32 value, index, tileStretchCount;
    Bool abbreviated, inscribed, tileStretch, tilingPresent;
    if (input == NULL || result == NULL) return FALSE;
    memset(result, 0, sizeof(*result));
    if (!read_bits(input, 4, &value) ||
        value != CODEC_VERSION) return FALSE;
    result->codecVersion = (U8)value;
    if (!read_bits(input, 4, &value) || (value != CODEC_SUBVERSION &&
        value != CODEC_SUBVERSION_NEWSCALING_SOFT_TILES &&
        value != CODEC_SUBVERSION_NEWSCALING_HARD_TILES)) return FALSE;
    result->codecSubVersion = (U8)value;
    result->useHardTileBoundaries = value == CODEC_SUBVERSION_NEWSCALING_HARD_TILES;

    if (!read_bits(input, 1, &value)) return FALSE;
    tilingPresent = (Bool)value;
    if (!read_bits(input, 1, &value)) return FALSE;
    result->bitstreamFormat = (BITSTREAMFORMAT)value;
    if (!read_bits(input, 3, &value)) return FALSE;
    result->orientation = (ORIENTATION)value;
    if (!read_bits(input, 1, &value)) return FALSE;
    result->hasIndexTable = (Bool)value;
    if (!read_bits(input, 2, &value) || value == 3) return FALSE;
    result->overlap = (OVERLAP)value;

    if (!read_bits(input, 1, &value)) return FALSE;
    abbreviated = (Bool)value;
    if (!read_bits(input, 1, &value)) return FALSE;
    result->codedBitDepth = (BITDEPTH)value;
    if (!read_bits(input, 1, &value)) return FALSE;
    inscribed = (Bool)value;
    if (!read_bits(input, 1, &value)) return FALSE;
    result->trimFlexbits = (Bool)value;
    if (!read_bits(input, 1, &value)) return FALSE;
    tileStretch = (Bool)value;
    if (!read_bits(input, 1, &value)) return FALSE;
    result->redBlueSwapped = (Bool)value;
    if (!read_bits(input, 1, &value) || !read_bits(input, 1, &value)) return FALSE;
    result->hasAlphaChannel = (Bool)value;
    if (!read_bits(input, 4, &value)) return FALSE;
    result->sourceColorFormat = (COLORFORMAT)value;
    if (!read_bits(input, 4, &value)) return FALSE;
    result->blackWhite = value == BD_1alt;
    result->sourceBitDepth = result->blackWhite ? BD_1 : (BITDEPTH_BITS)value;
    if (!read_bits(input, abbreviated ? 16 : 32, &value)) return FALSE;
    result->width = (size_t)value + 1;
    if (!read_bits(input, abbreviated ? 16 : 32, &value)) return FALSE;
    result->height = (size_t)value + 1;

    result->extraPixelsTop = result->extraPixelsLeft = 0;
    result->extraPixelsBottom = inscribed ? 0 : ((16 - (result->height & 15)) & 15);
    result->extraPixelsRight = inscribed ? 0 : ((16 - (result->width & 15)) & 15);
    result->verticalSliceCountMinusOne = result->horizontalSliceCountMinusOne = 0;
    if (tilingPresent) {
        if (!read_bits(input, LOG_MAX_TILES, &value)) return FALSE;
        result->verticalSliceCountMinusOne = value;
        if (!read_bits(input, LOG_MAX_TILES, &value)) return FALSE;
        result->horizontalSliceCountMinusOne = value;
    }
    if ((!result->hasIndexTable && (result->bitstreamFormat == FREQUENCY ||
        result->verticalSliceCountMinusOne + result->horizontalSliceCountMinusOne > 0)) ||
        result->verticalSliceCountMinusOne >= MAX_TILES ||
        result->horizontalSliceCountMinusOne >= MAX_TILES) return FALSE;
    if (!read_tile_sizes(input, abbreviated, result->verticalSliceCountMinusOne, result->tileX) ||
        !read_tile_sizes(input, abbreviated, result->horizontalSliceCountMinusOne, result->tileY)) return FALSE;
    if (tileStretch) {
        tileStretchCount = (result->verticalSliceCountMinusOne + 1) *
            (result->horizontalSliceCountMinusOne + 1);
        for (index = 0; index < tileStretchCount; ++index)
            if (!read_bits(input, 8, &value)) return FALSE;
    }
    if (inscribed) {
        if (!read_bits(input, 6, &value)) return FALSE; result->extraPixelsTop = value;
        if (!read_bits(input, 6, &value)) return FALSE; result->extraPixelsLeft = value;
        if (!read_bits(input, 6, &value)) return FALSE; result->extraPixelsBottom = value;
        if (!read_bits(input, 6, &value)) return FALSE; result->extraPixelsRight = value;
    }
    if (((result->width + result->extraPixelsLeft + result->extraPixelsRight) & 15) +
        ((result->height + result->extraPixelsTop + result->extraPixelsBottom) & 15) != 0) {
        if ((result->width & 15) + (result->height & 15) + result->extraPixelsLeft +
            result->extraPixelsTop != 0 || result->width <= result->extraPixelsRight ||
            result->height <= result->extraPixelsBottom) return FALSE;
        result->width -= result->extraPixelsRight;
        result->height -= result->extraPixelsBottom;
    }
    return TRUE;
}
