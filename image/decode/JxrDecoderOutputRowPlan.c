#include "JxrDecoderOutputRowPlan.h"
#include "decode.h"
#include "JxrDecoderRoiRowRange.h"
#include <assert.h>

static COLORFORMAT JxrDecoderOutputRowPlanGetOutputColorFormat(
    const CWMImageStrCodec* codec)
{
    return codec->m_param.cfColorFormat == Y_ONLY ? Y_ONLY : codec->WMII.cfColorFormat;
}

static size_t JxrDecoderOutputRowPlanGetFirstSourceRow(
    const CWMImageStrCodec* codec)
{
    return (codec->cRow - 1) * 16 > codec->m_Dparam->cROITopY ? 0 :
        (codec->m_Dparam->cROITopY & 0xf);
}

static size_t JxrDecoderOutputRowPlanGetThumbnailBits(size_t thumbnailScale)
{
    size_t thumbnailBits = 0;

    while ((size_t)(1U << thumbnailBits) < thumbnailScale)
        thumbnailBits++;

    assert(thumbnailScale == (size_t)(1U << thumbnailBits));
    return thumbnailBits;
}

Void JxrDecoderOutputRowPlanInitializeStandard(JxrDecoderOutputRowPlan* plan,
    const CWMImageStrCodec* codec)
{
    plan->internalColorFormat = codec->m_param.cfColorFormat;
    plan->outputColorFormat = JxrDecoderOutputRowPlanGetOutputColorFormat(codec);
    plan->bitDepth = codec->WMII.bdBitDepth;
    plan->outputHeight = JxrDecoderRoiRowRangeGetOutputHeight(
        codec->m_Dparam->cROIBottomY + 1, codec->cRow);
    plan->outputWidth = codec->m_Dparam->cROIRightX + 1;
    plan->firstRow = JxrDecoderOutputRowPlanGetFirstSourceRow(codec);
    plan->firstColumn = codec->m_Dparam->cROILeftX;
    plan->thumbnailScale = 1;
    plan->thumbnailBits = 0;
}

Void JxrDecoderOutputRowPlanInitializeThumbnail(JxrDecoderOutputRowPlan* plan,
    const CWMImageStrCodec* codec)
{
    const size_t thumbnailScale = codec->m_Dparam->cThumbnailScale;
    const size_t firstSourceRow = JxrDecoderOutputRowPlanGetFirstSourceRow(codec);

    plan->internalColorFormat = codec->m_param.cfColorFormat;
    plan->outputColorFormat = JxrDecoderOutputRowPlanGetOutputColorFormat(codec);
    plan->bitDepth = codec->WMII.bdBitDepth;
    plan->outputHeight = JxrDecoderRoiRowRangeGetOutputHeight(
        codec->m_Dparam->bDecodeFullFrame ? codec->WMII.cHeight :
        codec->m_Dparam->cROIBottomY + 1, codec->cRow);
    plan->outputWidth = codec->m_Dparam->bDecodeFullFrame ? codec->WMII.cWidth :
        codec->m_Dparam->cROIRightX + 1;
    plan->firstRow = (firstSourceRow + thumbnailScale - 1) /
        thumbnailScale * thumbnailScale;
    plan->firstColumn = (codec->m_Dparam->cROILeftX + thumbnailScale - 1) /
        thumbnailScale * thumbnailScale;
    plan->thumbnailScale = thumbnailScale;
    plan->thumbnailBits = JxrDecoderOutputRowPlanGetThumbnailBits(thumbnailScale);
}

Void JxrDecoderOutputRowPlanInitializeThumbnailNChannel(JxrDecoderOutputRowPlan* plan,
    const CWMImageStrCodec* codec)
{
    const size_t thumbnailScale = codec->m_Dparam->cThumbnailScale;
    const size_t firstSourceRow = JxrDecoderOutputRowPlanGetFirstSourceRow(codec);

    plan->internalColorFormat = codec->m_param.cfColorFormat;
    plan->outputColorFormat = JxrDecoderOutputRowPlanGetOutputColorFormat(codec);
    plan->bitDepth = codec->WMII.bdBitDepth;
    plan->outputHeight = JxrDecoderRoiRowRangeGetOutputHeight(
        codec->m_Dparam->cROIBottomY + 1, codec->cRow);
    plan->outputWidth = codec->m_Dparam->cROIRightX + 1;
    plan->firstRow = (firstSourceRow + thumbnailScale - 1) /
        thumbnailScale * thumbnailScale;
    plan->firstColumn = (codec->m_Dparam->cROILeftX + thumbnailScale - 1) /
        thumbnailScale * thumbnailScale;
    plan->thumbnailScale = thumbnailScale;
    plan->thumbnailBits = JxrDecoderOutputRowPlanGetThumbnailBits(thumbnailScale);
}
