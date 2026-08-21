#include "JxrEncoderMainHeaderWriter.h"

#include "JxrEncoderImagePlaneHeaderWriter.h"

Void JxrEncoderMainHeaderPlanInitialize(
    JxrEncoderMainHeaderPlan* plan,
    size_t width,
    size_t height,
    U32 verticalSliceCountMinusOne,
    U32 horizontalSliceCountMinusOne,
    size_t extraPixelsTop,
    size_t extraPixelsLeft,
    size_t extraPixelsBottom,
    size_t extraPixelsRight,
    BITDEPTH_BITS bitDepth,
    Bool blackWhite,
    Bool usesHardTileBoundaries)
{
    plan->usesAbbreviatedFields = (width + 15) / 16 <= 255 &&
        (height + 15) / 16 <= 255;
    plan->writesTiling = verticalSliceCountMinusOne != 0 ||
        horizontalSliceCountMinusOne != 0;
    plan->writesWindowing = extraPixelsTop != 0 || extraPixelsLeft != 0 ||
        extraPixelsBottom != 0 || extraPixelsRight != 0;
    plan->usesAlternateOneBitDepth = bitDepth == BD_1 && blackWhite;
    plan->usesHardTileSubversion = usesHardTileBoundaries;
    plan->tileSizeBitCount = plan->usesAbbreviatedFields ? 8 : 16;
}

static Void JxrEncoderMainHeaderWriterClearNonTranscodeWindow(
    CCoreParameters* coreParameters)
{
    if (!coreParameters->bTranscode)
        coreParameters->cExtraPixelsTop = coreParameters->cExtraPixelsLeft =
            coreParameters->cExtraPixelsRight = coreParameters->cExtraPixelsBottom = 0;
}

static Void JxrEncoderMainHeaderWriterWriteSignature(BitIOInfo* bitWriter)
{
    U32 signatureIndex;

    for (signatureIndex = 0; signatureIndex < 8; ++signatureIndex)
        putBit16(bitWriter, gGDISignature[signatureIndex], 8);
}

static Void JxrEncoderMainHeaderWriterWriteTileSizes(
    const CWMIStrCodecParam* parameters,
    BitIOInfo* bitWriter,
    const JxrEncoderMainHeaderPlan* plan)
{
    U32 tileIndex;

    for (tileIndex = 0; tileIndex < parameters->cNumOfSliceMinus1V; ++tileIndex)
        putBit16(bitWriter, parameters->uiTileX[tileIndex + 1] -
            parameters->uiTileX[tileIndex], plan->tileSizeBitCount);
    for (tileIndex = 0; tileIndex < parameters->cNumOfSliceMinus1H; ++tileIndex)
        putBit16(bitWriter, parameters->uiTileY[tileIndex + 1] -
            parameters->uiTileY[tileIndex], plan->tileSizeBitCount);
}

Int JxrEncoderMainHeaderWriterWrite(CWMImageStrCodec* codec)
{
    CWMImageInfo* image = &codec->WMII;
    CWMIStrCodecParam* parameters = &codec->WMISCP;
    CCoreParameters* coreParameters = &codec->m_param;
    BitIOInfo* bitWriter = codec->pIOHeader;
    JxrEncoderMainHeaderPlan plan;

    JxrEncoderMainHeaderWriterClearNonTranscodeWindow(coreParameters);
    JxrEncoderMainHeaderPlanInitialize(&plan, image->cWidth, image->cHeight,
        parameters->cNumOfSliceMinus1V, parameters->cNumOfSliceMinus1H,
        coreParameters->cExtraPixelsTop, coreParameters->cExtraPixelsLeft,
        coreParameters->cExtraPixelsBottom, coreParameters->cExtraPixelsRight,
        image->bdBitDepth, parameters->bBlackWhite,
        parameters->bUseHardTileBoundaries);

    JxrEncoderMainHeaderWriterWriteSignature(bitWriter);
    putBit16(bitWriter, CODEC_VERSION, 4);
    putBit16(bitWriter, plan.usesHardTileSubversion ?
        CODEC_SUBVERSION_NEWSCALING_HARD_TILES :
        CODEC_SUBVERSION_NEWSCALING_SOFT_TILES, 4);
    putBit16(bitWriter, plan.writesTiling ? 1 : 0, 1);
    putBit16(bitWriter, (Int)parameters->bfBitstreamFormat, 1);
    putBit16(bitWriter, image->oOrientation, 3);
    putBit16(bitWriter, coreParameters->bIndexTable, 1);
    putBit16(bitWriter, parameters->olOverlap, 2);
    putBit16(bitWriter, plan.usesAbbreviatedFields, 1);
    putBit16(bitWriter, 1, 1);
    putBit16(bitWriter, plan.writesWindowing, 1);
    putBit16(bitWriter, coreParameters->bTrimFlexbitsFlag, 1);
    putBit16(bitWriter, 0, 1);
    putBit16(bitWriter, 0, 2);
    putBit16(bitWriter, (Int)coreParameters->bAlphaChannel, 1);
    putBit16(bitWriter, (Int)image->cfColorFormat, 4);
    putBit16(bitWriter, plan.usesAlternateOneBitDepth ? BD_1alt : image->bdBitDepth, 4);
    putBit32(bitWriter, (U32)(image->cWidth - 1),
        plan.usesAbbreviatedFields ? 16 : 32);
    putBit32(bitWriter, (U32)(image->cHeight - 1),
        plan.usesAbbreviatedFields ? 16 : 32);

    if (plan.writesTiling) {
        putBit16(bitWriter, parameters->cNumOfSliceMinus1V, LOG_MAX_TILES);
        putBit16(bitWriter, parameters->cNumOfSliceMinus1H, LOG_MAX_TILES);
    }
    JxrEncoderMainHeaderWriterWriteTileSizes(parameters, bitWriter, &plan);
    if (plan.writesWindowing) {
        putBit16(bitWriter, (U32)coreParameters->cExtraPixelsTop, 6);
        putBit16(bitWriter, (U32)coreParameters->cExtraPixelsLeft, 6);
        putBit16(bitWriter, (U32)coreParameters->cExtraPixelsBottom, 6);
        putBit16(bitWriter, (U32)coreParameters->cExtraPixelsRight, 6);
    }
    fillToByte(bitWriter);
    JxrEncoderImagePlaneHeaderWriterWrite(codec);
    return ICERR_OK;
}
