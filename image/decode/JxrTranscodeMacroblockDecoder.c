#include "JxrTranscodeMacroblockDecoder.h"

#include "decode.h"
#include "JxrDecoderCoefficientPredictor.h"
#include "JxrDecoderPacketPipeline.h"

EXTERN_C Int DecodeMacroblockDC(CWMImageStrCodec*, CCodingContext*, Int, Int);
EXTERN_C Int DecodeMacroblockLowpass(CWMImageStrCodec*, CCodingContext*, Int, Int);
EXTERN_C Int DecodeMacroblockHighpass(CWMImageStrCodec*, CCodingContext*, Int, Int);

static Int JxrTranscodeMacroblockDecoderDecodePlane(CWMImageStrCodec* codec,
    Int macroblockColumn, Int macroblockRow)
{
    CCodingContext* context;

    getTilePos(codec, macroblockColumn, macroblockRow);
    if (JxrDecoderPacketPipelineReadCurrentMacroblock(codec) != ICERR_OK)
        return ICERR_ERROR;
    context = &codec->m_pCodingContext[codec->cTileColumn];
    if (DecodeMacroblockDC(codec, context, macroblockColumn, macroblockRow) != ICERR_OK)
        return ICERR_ERROR;
    if (codec->cSB > 1 &&
        DecodeMacroblockLowpass(codec, context, macroblockColumn, macroblockRow) != ICERR_OK)
        return ICERR_ERROR;
    JxrDecoderCoefficientPredictorApplyDcAc(codec);
    if (codec->cSB > 2 &&
        DecodeMacroblockHighpass(codec, context, macroblockColumn, macroblockRow) != ICERR_OK)
        return ICERR_ERROR;
    JxrDecoderCoefficientPredictorApplyAc(codec);
    updatePredInfo(codec, &codec->MBInfo, macroblockColumn, codec->WMISCP.cfColorFormat);
    return ICERR_OK;
}

Int JxrTranscodeMacroblockDecoderDecode(const JxrTranscodePlanePair* planes,
    Int macroblockColumn, Int macroblockRow)
{
    if (planes == NULL || planes->primaryCodec == NULL ||
        (planes->hasAlpha && planes->alphaCodec == NULL)) return ICERR_ERROR;
    if (planes->hasAlpha) {
        planes->alphaCodec->cTileColumn = planes->primaryCodec->cTileColumn;
        planes->alphaCodec->cTileRow = planes->primaryCodec->cTileRow;
    }
    if (JxrTranscodeMacroblockDecoderDecodePlane(planes->primaryCodec,
        macroblockColumn, macroblockRow) != ICERR_OK) return ICERR_ERROR;
    if (planes->hasAlpha && JxrTranscodeMacroblockDecoderDecodePlane(
        planes->alphaCodec, macroblockColumn, macroblockRow) != ICERR_OK)
        return ICERR_ERROR;
    return ICERR_OK;
}
