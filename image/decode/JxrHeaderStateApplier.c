#include "JxrHeaderStateApplier.h"
#include <string.h>

static Bool apply_quantizer_indices(U8 destination[MAX_CHANNELS],
    const U8 source[MAX_CHANNELS], U8 mode, size_t channelCount)
{
    size_t channel;
    if (channelCount == 0 || channelCount >= MAX_CHANNELS) return FALSE;
    destination[0] = source[0];
    if (mode == 1) {
        if (channelCount < 2) return FALSE;
        destination[1] = source[1];
    } else if (mode > 0) {
        for (channel = 1; channel < channelCount; ++channel)
            destination[channel] = source[channel];
    }
    return TRUE;
}

Bool JxrHeaderStateApplierApplyMain(const JxrMainHeaderDescriptor* source,
    CWMImageInfo* imageInfo, CWMIStrCodecParam* codecParameters,
    CCoreParameters* coreParameters)
{
    if (source == NULL || imageInfo == NULL || codecParameters == NULL ||
        coreParameters == NULL) return FALSE;
    coreParameters->cVersion = source->codecVersion;
    coreParameters->cSubVersion = source->codecSubVersion;
    coreParameters->bUseHardTileBoundaries = source->useHardTileBoundaries;
    codecParameters->bUseHardTileBoundaries = source->useHardTileBoundaries;
    codecParameters->bfBitstreamFormat = source->bitstreamFormat;
    imageInfo->oOrientation = source->orientation;
    coreParameters->bIndexTable = source->hasIndexTable;
    codecParameters->olOverlap = source->overlap;
    codecParameters->bdBitDepth = BD_LONG;
    coreParameters->bTrimFlexbitsFlag = source->trimFlexbits;
    coreParameters->bRBSwapped = source->redBlueSwapped;
    coreParameters->bAlphaChannel = source->hasAlphaChannel;
    imageInfo->cfColorFormat = source->sourceColorFormat;
    imageInfo->bdBitDepth = source->sourceBitDepth;
    if (source->blackWhite) codecParameters->bBlackWhite = TRUE;
    imageInfo->cWidth = source->width;
    imageInfo->cHeight = source->height;
    coreParameters->cExtraPixelsTop = source->extraPixelsTop;
    coreParameters->cExtraPixelsLeft = source->extraPixelsLeft;
    coreParameters->cExtraPixelsBottom = source->extraPixelsBottom;
    coreParameters->cExtraPixelsRight = source->extraPixelsRight;
    codecParameters->cNumOfSliceMinus1V = source->verticalSliceCountMinusOne;
    codecParameters->cNumOfSliceMinus1H = source->horizontalSliceCountMinusOne;
    memcpy(codecParameters->uiTileX, source->tileX, sizeof(source->tileX));
    memcpy(codecParameters->uiTileY, source->tileY, sizeof(source->tileY));
    return TRUE;
}

Bool JxrHeaderStateApplierApplyImagePlane(const JxrImagePlaneDescriptor* source,
    CWMImageInfo* imageInfo, CWMIStrCodecParam* codecParameters,
    CCoreParameters* coreParameters)
{
    if (source == NULL || imageInfo == NULL || codecParameters == NULL ||
        coreParameters == NULL || source->channelCount == 0 ||
        source->channelCount >= MAX_CHANNELS) return FALSE;
    coreParameters->cfColorFormat = source->colorFormat;
    codecParameters->cfColorFormat = source->colorFormat;
    coreParameters->bScaledArith = source->scaledArithmetic;
    codecParameters->sbSubband = source->subband;
    coreParameters->cNumChannels = source->channelCount;
    if (source->hasChromaCenteringX)
        imageInfo->cChromaCenteringX = source->chromaCenteringX;
    if (source->hasChromaCenteringY)
        imageInfo->cChromaCenteringY = source->chromaCenteringY;
    if (source->hasSampleConversion) {
        codecParameters->nLenMantissaOrShift = source->mantissaOrShift;
        if (imageInfo->bdBitDepth == BD_32F)
            codecParameters->nExpBias = source->exponentBias;
    }
    return TRUE;
}

Bool JxrHeaderStateApplierApplyImagePlaneQuantizers(
    const JxrImagePlaneQuantizerHeader* source, CCoreParameters* coreParameters)
{
    if (source == NULL || coreParameters == NULL || coreParameters->cNumChannels == 0 ||
        coreParameters->cNumChannels >= MAX_CHANNELS) return FALSE;
    coreParameters->uQPMode = source->quantizerMode;
    if (source->hasDc && !apply_quantizer_indices(coreParameters->uiQPIndexDC,
        source->dcIndices, source->dcMode, coreParameters->cNumChannels)) return FALSE;
    if (source->hasLp && !apply_quantizer_indices(coreParameters->uiQPIndexLP,
        source->lpIndices, source->lpMode, coreParameters->cNumChannels)) return FALSE;
    if (source->hasHp && !apply_quantizer_indices(coreParameters->uiQPIndexHP,
        source->hpIndices, source->hpMode, coreParameters->cNumChannels)) return FALSE;
    return TRUE;
}
