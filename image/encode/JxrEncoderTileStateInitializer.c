#include "JxrEncoderTileStateInitializer.h"
#include "JxrEncoderQuantizerInitializer.h"
#include "encode.h"

Void JxrEncoderTileStatePlanInitialize(JxrEncoderTileStatePlan* plan,
    U32 verticalSlicesMinusOne)
{
    plan->supportsTileCount = verticalSlicesMinusOne < MAX_TILES;
    plan->codingContextCount = plan->supportsTileCount ?
        (Int)(verticalSlicesMinusOne + 1) : 0;
}

Int JxrEncoderTileStateInitializerInitialize(CWMImageStrCodec* codec)
{
    JxrEncoderTileStatePlan plan;

    codec->cTileColumn = 0;
    codec->cTileRow = 0;
    JxrEncoderTileStatePlanInitialize(&plan, codec->WMISCP.cNumOfSliceMinus1V);
    if (!plan.supportsTileCount)
        return ICERR_ERROR;

    if (allocateTileInfo(codec) != ICERR_OK)
        return ICERR_ERROR;
    if (JxrEncoderQuantizerInitializerInitialize(codec) != ICERR_OK)
        return ICERR_ERROR;
    if (allocatePredInfo(codec) != ICERR_OK)
        return ICERR_ERROR;
    if (AllocateCodingContextEnc(codec, plan.codingContextCount,
        codec->WMISCP.uiTrimFlexBits) != ICERR_OK)
        return ICERR_ERROR;

    return ICERR_OK;
}
