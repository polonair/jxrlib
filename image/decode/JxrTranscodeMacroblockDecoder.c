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

Int JxrTranscodeMacroblockDecoderDecode(CWMImageStrCodec* primaryCodec,
    Int macroblockColumn, Int macroblockRow)
{
    CWMImageStrCodec* codec;
    size_t planeCount;
    size_t planeIndex;

    if (primaryCodec == NULL) return ICERR_ERROR;
    codec = primaryCodec;
    planeCount = primaryCodec->m_param.bAlphaChannel ? 2 : 1;
    for (planeIndex = 0; planeIndex < planeCount; ++planeIndex) {
        if (planeIndex == 0 && planeCount == 2) {
            if (primaryCodec->m_pNextSC == NULL) return ICERR_ERROR;
            primaryCodec->m_pNextSC->cTileColumn = primaryCodec->cTileColumn;
            primaryCodec->m_pNextSC->cTileRow = primaryCodec->cTileRow;
        }
        if (JxrTranscodeMacroblockDecoderDecodePlane(codec, macroblockColumn,
            macroblockRow) != ICERR_OK) return ICERR_ERROR;
        codec = codec->m_pNextSC;
    }
    return ICERR_OK;
}
