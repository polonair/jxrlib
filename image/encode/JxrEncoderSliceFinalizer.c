#include "JxrEncoderSliceFinalizer.h"

#include "encode.h"

Void JxrEncoderSliceFinalizationPlanInitialize(
    JxrEncoderSliceFinalizationPlan* plan,
    const CWMImageStrCodec* codec,
    Int macroblockX,
    Int macroblockY)
{
    Bool completesImageRow = macroblockY + 1 == (Int)codec->cmbHeight;
    Bool completesTileSlice = codec->cTileRow < codec->WMISCP.cNumOfSliceMinus1H &&
        macroblockY == (Int)codec->WMISCP.uiTileY[codec->cTileRow + 1] - 1;

    plan->completesHorizontalSlice = macroblockX + 1 == (Int)codec->cmbWidth &&
        (completesImageRow || completesTileSlice);
    plan->updatesPacketIndex = plan->completesHorizontalSlice &&
        (codec->m_pNextSC == NULL || codec->m_bSecondary);
    plan->resetsCodingContexts = plan->completesHorizontalSlice && !completesImageRow;
}

static Void JxrEncoderSliceFinalizerUpdatePacketIndex(CWMImageStrCodec* codec)
{
    size_t bitStreamIndex;

    for (bitStreamIndex = 0; bitStreamIndex < codec->cNumBitIO; ++bitStreamIndex) {
        size_t streamPosition;

        fillToByte(codec->m_ppBitIO[bitStreamIndex]);
        codec->ppWStream[bitStreamIndex]->GetPos(
            codec->ppWStream[bitStreamIndex], &streamPosition);
        codec->pIndexTable[codec->cNumBitIO * codec->cTileRow + bitStreamIndex] =
            streamPosition + getSizeWrite(codec->m_ppBitIO[bitStreamIndex]);
    }
}

static Void JxrEncoderSliceFinalizerResetCodingContexts(CWMImageStrCodec* codec)
{
    size_t contextIndex;

    for (contextIndex = 0;
        contextIndex <= codec->WMISCP.cNumOfSliceMinus1V;
        ++contextIndex)
        ResetCodingContextEnc(&codec->m_pCodingContext[contextIndex]);
}

Void JxrEncoderSliceFinalizerFinalize(
    CWMImageStrCodec* codec,
    Int macroblockX,
    Int macroblockY)
{
    JxrEncoderSliceFinalizationPlan plan;

    JxrEncoderSliceFinalizationPlanInitialize(&plan, codec, macroblockX, macroblockY);
    if (!plan.completesHorizontalSlice)
        return;
    if (plan.updatesPacketIndex)
        JxrEncoderSliceFinalizerUpdatePacketIndex(codec);
    if (plan.resetsCodingContexts)
        JxrEncoderSliceFinalizerResetCodingContexts(codec);
}
