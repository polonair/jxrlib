#include "JxrDecoderOutputPipeline.h"
#include "decode.h"
#include "JxrInverseColorTransform.h"
#include "JxrSampleClipping.h"
#include "JxrFloatSampleConversion.h"
#include "JxrMonochromeExpansion.h"
#include "JxrDecoderRoiRowRange.h"
#include "JxrDecoderOutputRowPlan.h"
#include "JxrDecoderUvInterpolator.h"
#include "JxrDecoderNChannelOutputWriter.h"
#include "JxrDecoderStandardColorOutputWriter.h"
#include "JxrDecoderThumbnailNChannelOutputWriter.h"
#include "JxrDecoderThumbnailColorOutputWriter.h"
#include "JxrDecoderAlphaOutputWriter.h"
#include "JxrDecoderThumbnailAlphaOutputWriter.h"
#include "JXRTrace.h"

Void JxrDecoderOutputPipelinePlanInitialize(JxrDecoderOutputPipelinePlan* plan,
    Bool hasOptimizedLoadOverride)
{
    plan->usesLegacyLoadCallback = hasOptimizedLoadOverride;
}

// write one MB row of Y_ONLY/CF_ALPHA/YUV_444/N_CHANNEL to output buffer
Int JxrDecoderOutputPipelineWriteStandardRow(CWMImageStrCodec * pSC)
{
    JxrDecoderOutputRowPlan outputPlan;
    const PixelI iShift = (pSC->m_param.bScaledArith ? SHIFTZERO + QPFRACBITS : 0);
    COLORFORMAT cfExt;
    size_t cHeight, cWidth, iFirstRow, iFirstColumn;
    JxrDecoderOutputRowPlanInitializeStandard(&outputPlan, pSC);
    cfExt = outputPlan.outputColorFormat;
    cHeight = outputPlan.outputHeight;
    cWidth = outputPlan.outputWidth;
    iFirstRow = outputPlan.firstRow;
    iFirstColumn = outputPlan.firstColumn;

    size_t * pOffsetX = pSC->m_Dparam->pOffsetX;
    size_t * pOffsetY = pSC->m_Dparam->pOffsetY +
        (pSC->cRow - 1) * (cfExt == YUV_420 ? 8 : 16);


    if (pSC->m_pNextSC) {
        assert (pSC->m_param.bScaledArith == pSC->m_pNextSC->m_param.bScaledArith);  // will be relaxed later
    }

    // guard output buffer
    if(checkImageBuffer(pSC, pSC->WMII.oOrientation >= O_RCW ? pSC->WMII.cROIHeight : pSC->WMII.cROIWidth, cHeight - iFirstRow) != ICERR_OK)
        return ICERR_ERROR;

    if(pSC->m_bUVResolutionChange)
        JxrDecoderUvInterpolatorInterpolate(pSC);

    /* Trace the actual overlap-reconstructed macroblock row, after it is
     * available to the output writer. The transform callback runs one row
     * earlier and observes stale a0MBbuffer contents at the top edge. */
    if (JXRTraceEnabled() && pSC->cRow > 0 &&
        pSC->WMISCP.cfColorFormat == YUV_444 &&
        !pSC->m_bUVResolutionChange && pSC->m_param.cNumChannels >= 3)
    {
        size_t macroblockX;
        for (macroblockX = 0; macroblockX < pSC->cmbWidth; ++macroblockX)
        {
            size_t offset = macroblockX * 256;
            JXRTraceDumpValues("decoder", "reconstructed_samples",
                (Int)macroblockX, (Int)(pSC->cRow - 1), "Y",
                pSC->a0MBbuffer[0] + offset);
            JXRTraceDumpValues("decoder", "reconstructed_samples",
                (Int)macroblockX, (Int)(pSC->cRow - 1), "U",
                pSC->a0MBbuffer[1] + offset);
            JXRTraceDumpValues("decoder", "reconstructed_samples",
                (Int)macroblockX, (Int)(pSC->cRow - 1), "V",
                pSC->a0MBbuffer[2] + offset);
        }
    }

    JxrDecoderStandardColorOutputWriterWrite(pSC, &outputPlan, iShift);

    if(pSC->WMISCP.uAlphaMode > 0)
        if(JxrDecoderAlphaOutputWriterWrite(pSC) != ICERR_OK)
            return ICERR_ERROR;

#ifdef REENTRANT_MODE
    pSC->WMIBI.cLinesDecoded = cHeight - iFirstRow;

    if (CF_RGB == pSC->WMII.cfColorFormat && Y_ONLY == pSC->WMISCP.cfColorFormat)
    {
        const CWMImageInfo* pII = &pSC->WMII;

        switch (pII->bdBitDepth)
        {
            case BD_8:
                JxrMonochromeExpansionReplicateByteAtOffsets((U8*)pSC->WMIBI.pv, pOffsetX, pOffsetY, iFirstRow, cHeight, iFirstColumn, cWidth);
                break;

            case BD_16:
            case BD_16S:
            case BD_16F:
                JxrMonochromeExpansionReplicateUInt16AtOffsets((U16*)pSC->WMIBI.pv, pOffsetX, pOffsetY, iFirstRow, cHeight, iFirstColumn, cWidth);
                break;

            case BD_32:
            case BD_32S:
            case BD_32F:
                JxrMonochromeExpansionReplicateUInt32AtOffsets((U32*)pSC->WMIBI.pv, pOffsetX, pOffsetY, iFirstRow, cHeight, iFirstColumn, cWidth);
                break;

            case BD_5:
            case BD_10:
            case BD_565:
            default:
                break;
        }
    }
#endif

    return ICERR_OK;
}

Void JxrDecoderOutputPipelineFinalize(
    const CWMImageStrCodec* pSC,
    const CWMImageBufferInfo* pBI)
{
    const CWMImageInfo* pII = &pSC->WMII;
    const CWMIStrCodecParam* pSCP = &pSC->WMISCP;
    size_t cWidth = 0, cHeight = 0;

    if (CF_RGB != pII->cfColorFormat || Y_ONLY != pSCP->cfColorFormat)
        return;

    cWidth = 0 != pII->cROIWidth ? pII->cROIWidth : pII->cWidth;
    cHeight = 0 != pII->cROIHeight ? pII->cROIHeight : pII->cHeight;

    switch (pII->bdBitDepth)
    {
        case BD_8:
            JxrMonochromeExpansionReplicateByte((U8*)pBI->pv, pBI->cbStride, cWidth, cHeight, pII->cBitsPerUnit >> 3);
            break;

        case BD_16:
        case BD_16S:
        case BD_16F:
            JxrMonochromeExpansionReplicateUInt16((U16*)pBI->pv, pBI->cbStride, cWidth, cHeight, (pII->cBitsPerUnit >> 3) / sizeof(U16));
            break;

        case BD_32:
        case BD_32S:
        case BD_32F:
            JxrMonochromeExpansionReplicateUInt32((U32*)pBI->pv, pBI->cbStride, cWidth, cHeight, (pII->cBitsPerUnit >> 3) / sizeof(float));
            break;

        case BD_5:
        case BD_10:
        case BD_565:
        default:
            break;
    }
}

// centralized alpha channel thumbnail, small perf penalty
Int JxrDecoderOutputPipelineWriteThumbnailRow(CWMImageStrCodec * pSC)
{
    JxrDecoderOutputRowPlan outputPlan;
    JxrDecoderOutputRowPlan thumbnailNChannelPlan;
    size_t tScale, cHeight, cWidth, iFirstRow, iFirstColumn;
    COLORFORMAT cfInt;
    const OVERLAP ol = pSC->WMISCP.olOverlap;
	const size_t iB = (pSC->WMII.bRGB ? 2 : 0);
    const size_t iR = 2 - iB;

    size_t nBits;

    JxrDecoderOutputRowPlanInitializeThumbnail(&outputPlan, pSC);
    JxrDecoderOutputRowPlanInitializeThumbnailNChannel(&thumbnailNChannelPlan, pSC);
    tScale = outputPlan.thumbnailScale;
    cHeight = outputPlan.outputHeight;
    cWidth = outputPlan.outputWidth;
    iFirstRow = outputPlan.firstRow;
    iFirstColumn = outputPlan.firstColumn;
    cfInt = outputPlan.internalColorFormat;
    nBits = outputPlan.thumbnailBits;

    size_t * pOffsetX = pSC->m_Dparam->pOffsetX;
    size_t * pOffsetY = pSC->m_Dparam->pOffsetY +
        (pSC->cRow - 1) * 16 / tScale;
    const PixelI cMul = (tScale >= 16 ? (ol == OL_NONE ? 16 : (ol == OL_ONE ? 23 : 34)) : (tScale >= 4 ? (ol == OL_NONE ? 64 : 93) : 258));
    const size_t rShiftY = 8 + (pSC->m_param.bScaledArith ? (SHIFTZERO + QPFRACBITS) : 0);
    const size_t rShiftUV = rShiftY - ((pSC->m_param.bScaledArith && tScale >= 16) ? ((cfInt == YUV_420 || cfInt == YUV_422) ? 2 : 1) : 0);

    // guard output buffer
    if(checkImageBuffer(pSC, pSC->WMII.oOrientation < O_RCW ? pSC->WMII.cROIWidth : pSC->WMII.cROIHeight, (cHeight - iFirstRow) / pSC->m_Dparam->cThumbnailScale) != ICERR_OK)
        return ICERR_ERROR;

    if((((pSC->cRow - 1) * 16) % tScale) != 0)
        return ICERR_OK;

    if(pSC->cRow * 16 <= pSC->m_Dparam->cROITopY || pSC->cRow * 16 > pSC->m_Dparam->cROIBottomY + 16)
        return ICERR_OK;

    JxrDecoderThumbnailColorOutputWriterWrite(pSC, &outputPlan, &thumbnailNChannelPlan, cMul, rShiftY, rShiftUV);

    if(pSC->WMISCP.uAlphaMode > 0)
        if(JxrDecoderThumbnailAlphaOutputWriterWrite(pSC, nBits, cMul, rShiftY) != ICERR_OK)
            return ICERR_ERROR;

#ifdef REENTRANT_MODE
    pSC->WMIBI.cLinesDecoded = ( cHeight - iFirstRow + tScale - 1 ) / tScale;
    if (CF_RGB == pSC->WMII.cfColorFormat && Y_ONLY == pSC->WMISCP.cfColorFormat)
    {
        const CWMImageInfo* pII = &pSC->WMII;

        switch (pII->bdBitDepth)
        {
            case BD_8:
                JxrMonochromeExpansionReplicateByteAtScaledOffsets((U8*)pSC->WMIBI.pv, pOffsetX, pOffsetY, iFirstRow, cHeight, iFirstColumn, cWidth, tScale, nBits, iR, iB);
                break;

            case BD_16:
            case BD_16S:
            case BD_16F:
                JxrMonochromeExpansionReplicateUInt16AtScaledOffsets((U16*)pSC->WMIBI.pv, pOffsetX, pOffsetY, iFirstRow, cHeight, iFirstColumn, cWidth, tScale, nBits, iR, iB);
                break;

            case BD_32:
            case BD_32S:
            case BD_32F:
                JxrMonochromeExpansionReplicateUInt32AtScaledOffsets((U32*)pSC->WMIBI.pv, pOffsetX, pOffsetY, iFirstRow, cHeight, iFirstColumn, cWidth, tScale, nBits, iR, iB);
                break;

            case BD_5:
            case BD_10:
            case BD_565:
            default:
                break;
        }
    }
#endif

    return ICERR_OK;
}
