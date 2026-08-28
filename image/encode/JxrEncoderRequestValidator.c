#include "JxrEncoderRequestValidator.h"

U32 JxrEncoderRequestValidatorSetUniformTiling(U32* tileSizes,
    U32 tileCount, U32 macroblockCount)
{
    U32 tileIndex;
    U32 remainingMacroblocks;

    while ((macroblockCount + tileCount - 1) / tileCount > 65535)
        ++tileCount;

    remainingMacroblocks = macroblockCount;
    for (tileIndex = tileCount; tileIndex > 1; --tileIndex) {
        U32 outputIndex = tileCount - tileIndex;
        tileSizes[outputIndex] = (remainingMacroblocks + tileIndex - 1) / tileIndex;
        remainingMacroblocks -= tileSizes[outputIndex];
    }

    return tileCount;
}

U32 JxrEncoderRequestValidatorNormalizeTiling(U32* tileBoundaries,
    U32 tileCount, U32 macroblockCount)
{
    U32 tileIndex;
    U32 macroblocksInPreviousTiles = 0;

    if (tileCount == 0)
        tileCount = 1;
    if (tileCount > macroblockCount)
        tileCount = 1;
    if (tileCount > MAX_TILES)
        tileCount = MAX_TILES;

    for (tileIndex = 0; tileIndex + 1 < tileCount; ++tileIndex) {
        if (tileBoundaries[tileIndex] == 0 || tileBoundaries[tileIndex] > 65535) {
            tileCount = JxrEncoderRequestValidatorSetUniformTiling(tileBoundaries,
                tileCount, macroblockCount);
            break;
        }

        macroblocksInPreviousTiles += tileBoundaries[tileIndex];
        if (macroblocksInPreviousTiles >= macroblockCount) {
            tileCount = tileIndex + 1;
            break;
        }
    }

    if (macroblockCount - macroblocksInPreviousTiles > 65536)
        tileCount = JxrEncoderRequestValidatorSetUniformTiling(tileBoundaries,
            tileCount, macroblockCount);

    for (tileIndex = 1; tileIndex < tileCount; ++tileIndex)
        tileBoundaries[tileIndex] += tileBoundaries[tileIndex - 1];
    for (tileIndex = tileCount - 1; tileIndex > 0; --tileIndex)
        tileBoundaries[tileIndex] = tileBoundaries[tileIndex - 1];
    tileBoundaries[0] = 0;

    return tileCount;
}

Int JxrEncoderRequestValidatorValidateAndNormalize(CWMImageInfo* imageInfo,
    CWMIStrCodecParam* codecParameters)
{
    int tileIndex;
    Bool hasTooNarrowHardTile = FALSE;
    U32 macroblockWidth;
    U32 macroblockHeight;

    if (imageInfo == NULL || codecParameters == NULL)
        return ICERR_ERROR;
    if (imageInfo->cWidth > (1 << 28) || imageInfo->cHeight > (1 << 28) ||
        imageInfo->cWidth == 0 || imageInfo->cHeight == 0) {
        printf("Unsurpported image size!\n");
        return ICERR_ERROR;
    }

    macroblockWidth = ((U32)imageInfo->cWidth + 15) >> 4;
    macroblockHeight = ((U32)imageInfo->cHeight + 15) >> 4;
    if ((codecParameters->cfColorFormat == YUV_420 ||
            codecParameters->cfColorFormat == YUV_422) &&
        codecParameters->olOverlap == OL_TWO && macroblockWidth < 2) {
        printf("Image width must be at least 2 MB wide for subsampled chroma and two levels of overlap!\n");
        return ICERR_ERROR;
    }

    if (codecParameters->sbSubband == SB_ISOLATED ||
        codecParameters->sbSubband >= SB_MAX)
        codecParameters->sbSubband = SB_ALL;

    if (imageInfo->bdBitDepth == BD_5 &&
        (imageInfo->cfColorFormat != CF_RGB || imageInfo->cBitsPerUnit != 16 ||
            imageInfo->cLeadingPadding != 0)) {
        printf("Unsupported BD_5 image format!\n");
        return ICERR_ERROR;
    }
    if (imageInfo->bdBitDepth == BD_565 &&
        (imageInfo->cfColorFormat != CF_RGB || imageInfo->cBitsPerUnit != 16 ||
            imageInfo->cLeadingPadding != 0)) {
        printf("Unsupported BD_565 image format!\n");
        return ICERR_ERROR;
    }
    if (imageInfo->bdBitDepth == BD_10 &&
        (imageInfo->cfColorFormat != CF_RGB || imageInfo->cBitsPerUnit != 32 ||
            imageInfo->cLeadingPadding != 0)) {
        printf("Unsupported BD_10 image format!\n");
        return ICERR_ERROR;
    }

    if ((imageInfo->bdBitDepth == BD_5 || imageInfo->bdBitDepth == BD_565 ||
            imageInfo->bdBitDepth == BD_10) &&
        codecParameters->cfColorFormat != YUV_420 &&
        codecParameters->cfColorFormat != YUV_422 &&
        codecParameters->cfColorFormat != Y_ONLY)
        codecParameters->cfColorFormat = YUV_444;

    if (imageInfo->bdBitDepth == BD_1) {
        if (imageInfo->cfColorFormat != Y_ONLY) {
            printf("BD_1 image must be black-and white!\n");
            return ICERR_ERROR;
        }
        codecParameters->cfColorFormat = Y_ONLY;
    }

    if (codecParameters->bdBitDepth != BD_LONG)
        codecParameters->bdBitDepth = BD_LONG;

    if (codecParameters->uAlphaMode > 1 &&
        (imageInfo->cfColorFormat == YUV_420 || imageInfo->cfColorFormat == YUV_422 ||
            imageInfo->bdBitDepth == BD_5 || imageInfo->bdBitDepth == BD_10 ||
            imageInfo->bdBitDepth == BD_1)) {
        printf("Alpha is not supported for this pixel format!\n");
        return ICERR_ERROR;
    }

    if ((codecParameters->cfColorFormat == YUV_420 ||
            codecParameters->cfColorFormat == YUV_422) &&
        (imageInfo->bdBitDepth == BD_16F || imageInfo->bdBitDepth == BD_32F ||
            imageInfo->cfColorFormat == CF_RGBE)) {
        printf("Float or RGBE images must be encoded with YUV 444!\n");
        return ICERR_ERROR;
    }

    codecParameters->cNumOfSliceMinus1V =
        JxrEncoderRequestValidatorNormalizeTiling(codecParameters->uiTileX,
            codecParameters->cNumOfSliceMinus1V + 1, macroblockWidth) - 1;
    codecParameters->cNumOfSliceMinus1H =
        JxrEncoderRequestValidatorNormalizeTiling(codecParameters->uiTileY,
            codecParameters->cNumOfSliceMinus1H + 1, macroblockHeight) - 1;

    if (codecParameters->bUseHardTileBoundaries &&
        (codecParameters->cfColorFormat == YUV_420 ||
            codecParameters->cfColorFormat == YUV_422) &&
        codecParameters->olOverlap == OL_TWO) {
        for (tileIndex = 1;
                tileIndex < (int)(codecParameters->cNumOfSliceMinus1H + 1);
                ++tileIndex) {
            if ((Int)(codecParameters->uiTileY[tileIndex] -
                    codecParameters->uiTileY[tileIndex - 1]) < 2) {
                hasTooNarrowHardTile = TRUE;
                break;
            }
        }
        if ((Int)(macroblockWidth -
                codecParameters->uiTileY[codecParameters->cNumOfSliceMinus1H]) < 2)
            hasTooNarrowHardTile = TRUE;
    }
    if (hasTooNarrowHardTile) {
        printf("Tile width must be at least 2 MB wide for hard tiles, subsampled chroma, and two levels of overlap!\n");
        return ICERR_ERROR;
    }

    if (codecParameters->cChannel > MAX_CHANNELS)
        return ICERR_ERROR;

    if ((imageInfo->cfColorFormat == Y_ONLY &&
            codecParameters->cfColorFormat != Y_ONLY) ||
        (codecParameters->cfColorFormat == YUV_422 &&
            (imageInfo->cfColorFormat == YUV_420 || imageInfo->cfColorFormat == Y_ONLY)) ||
        (codecParameters->cfColorFormat == YUV_444 &&
            (imageInfo->cfColorFormat == YUV_422 || imageInfo->cfColorFormat == YUV_420 ||
                imageInfo->cfColorFormat == Y_ONLY)))
        codecParameters->cfColorFormat = imageInfo->cfColorFormat;
    else if (imageInfo->cfColorFormat == NCOMPONENT)
        codecParameters->cfColorFormat = NCOMPONENT;
    if (imageInfo->cfColorFormat == CMYK && codecParameters->cfColorFormat == NCOMPONENT)
        codecParameters->cfColorFormat = CMYK;

    if (codecParameters->cfColorFormat != NCOMPONENT) {
        if (codecParameters->cfColorFormat == Y_ONLY)
            codecParameters->cChannel = 1;
        else if (codecParameters->cfColorFormat == CMYK)
            codecParameters->cChannel = 4;
        else
            codecParameters->cChannel = 3;
    }

    if (codecParameters->sbSubband >= SB_MAX)
        codecParameters->sbSubband = SB_ALL;

    imageInfo->cChromaCenteringX = 0;
    imageInfo->cChromaCenteringY = 0;
    return ICERR_OK;
}
