#include "JxrDecoderMacroblockProcessingPipeline.h"
#include "JxrDecoderDequantizer.h"

#include "JXRTrace.h"
#include "decode.h"
#include "JxrDecoderTransformPipeline.h"
#include "JxrDecoderPacketPipeline.h"
#include "JxrMacroblockRegionState.h"

Int JxrDecoderMacroblockProcessingPipelineProcess(CWMImageStrCodec* codec)
{
    const Bool hasSecondaryCodec = codec->m_pNextSC != NULL;
    CWMImageStrCodec* currentCodec = codec;
    const Bool isBottomRow = codec->cRow == codec->cmbHeight;
    const Bool isBottomRowOrRightColumn = isBottomRow || codec->cColumn == codec->cmbWidth;
    ERR_CODE result = ICERR_OK;
    size_t planeIndex;

    for (planeIndex = 0; planeIndex <= hasSecondaryCodec; ++planeIndex) {
        JxrMacroblockRegionState region;
        region.macroblockX = currentCodec->cColumn;
        region.macroblockY = currentCodec->cRow;
        region.roiLeftPixels = currentCodec->m_Dparam->cROILeftX;
        region.roiTopPixels = currentCodec->m_Dparam->cROITopY;
        region.roiRightPixels = currentCodec->m_Dparam->cROIRightX;
        region.roiBottomPixels = currentCodec->m_Dparam->cROIBottomY;

        if (!isBottomRowOrRightColumn) {
            CCodingContext* codingContext;

            getTilePos(currentCodec, currentCodec->cColumn, currentCodec->cRow);

            if (hasSecondaryCodec) {
                currentCodec->m_pNextSC->cTileColumn = currentCodec->cTileColumn;
                currentCodec->m_pNextSC->cTileRow = currentCodec->cTileRow;
            }

            codingContext = &currentCodec->m_pCodingContext[currentCodec->cTileColumn];
            if (JxrDecoderPacketPipelineReadCurrentMacroblock(currentCodec) != ICERR_OK)
                return ICERR_ERROR;

            region.tileLeftMacroblock = currentCodec->WMISCP.uiTileX[currentCodec->cTileColumn];
            region.tileTopMacroblock = currentCodec->WMISCP.uiTileY[currentCodec->cTileRow];
            region.tileRightMacroblock = currentCodec->cTileColumn !=
                currentCodec->WMISCP.cNumOfSliceMinus1V ?
                currentCodec->WMISCP.uiTileX[currentCodec->cTileColumn + 1] :
                currentCodec->cmbWidth;
            region.tileBottomMacroblock = currentCodec->cTileRow !=
                currentCodec->WMISCP.cNumOfSliceMinus1H ?
                currentCodec->WMISCP.uiTileY[currentCodec->cTileRow + 1] :
                currentCodec->cmbHeight;
            if (!currentCodec->m_Dparam->bDecodeFullFrame &&
                JxrMacroblockRegionStateIsTileStart(&region)) {
                codingContext->m_bInROI = JxrMacroblockRegionStateIntersectsEntropyRoi(&region,
                    currentCodec->WMISCP.olOverlap);
            }

            if (currentCodec->m_Dparam->bDecodeFullFrame || codingContext->m_bInROI) {
                size_t bitStart = JXRTraceBitPosition(codingContext->m_pIODC, FALSE);
                if ((result = DecodeMacroblockDC(currentCodec, codingContext,
                    (Int)currentCodec->cColumn, (Int)currentCodec->cRow)) != ICERR_OK) {
                    return result;
                }
                JXRTraceDumpBitRange("decoder", "dc", (Int)currentCodec->cColumn,
                    (Int)currentCodec->cRow, bitStart,
                    JXRTraceBitPosition(codingContext->m_pIODC, FALSE));
                JXRTraceDumpStage("decoder", "after_dc", currentCodec,
                    (Int)currentCodec->cColumn, (Int)currentCodec->cRow, JXRTraceCoefficients);

                if (currentCodec->m_Dparam->bDecodeLP) {
                    bitStart = JXRTraceBitPosition(codingContext->m_pIOLP, FALSE);
                    if ((result = DecodeMacroblockLowpass(currentCodec, codingContext,
                        (Int)currentCodec->cColumn, (Int)currentCodec->cRow)) != ICERR_OK) {
                        return result;
                    }
                    JXRTraceDumpBitRange("decoder", "lp", (Int)currentCodec->cColumn,
                        (Int)currentCodec->cRow, bitStart,
                        JXRTraceBitPosition(codingContext->m_pIOLP, FALSE));
                }

                JXRTraceDumpStage("decoder", "after_lp", currentCodec,
                    (Int)currentCodec->cColumn, (Int)currentCodec->cRow, JXRTraceCoefficients);
                predDCACDec(currentCodec);
                JXRTraceDumpStage("decoder", "after_dc_lp_prediction", currentCodec,
                    (Int)currentCodec->cColumn, (Int)currentCodec->cRow, JXRTraceCoefficients);

                JxrDecoderDequantizerDequantizeMacroblock(currentCodec);
                JXRTraceDumpStage("decoder", "after_dequantization", currentCodec,
                    (Int)currentCodec->cColumn, (Int)currentCodec->cRow, JXRTraceCoefficients);

                if (currentCodec->m_Dparam->bDecodeHP) {
                    bitStart = JXRTraceBitPosition(codingContext->m_pIOAC, FALSE);
                    if ((result = DecodeMacroblockHighpass(currentCodec, codingContext,
                        (Int)currentCodec->cColumn, (Int)currentCodec->cRow)) != ICERR_OK) {
                        return result;
                    }
                    JXRTraceDumpBitRange("decoder", "hp", (Int)currentCodec->cColumn,
                        (Int)currentCodec->cRow, bitStart,
                        JXRTraceBitPosition(codingContext->m_pIOAC, FALSE));
                    JXRTraceDumpStage("decoder", "after_hp", currentCodec,
                        (Int)currentCodec->cColumn, (Int)currentCodec->cRow, JXRTraceCoefficients);
                    predACDec(currentCodec);
                    JXRTraceDumpStage("decoder", "after_ac_prediction", currentCodec,
                        (Int)currentCodec->cColumn, (Int)currentCodec->cRow, JXRTraceCoefficients);
                }

                updatePredInfo(currentCodec, &currentCodec->MBInfo,
                    (Int)currentCodec->cColumn, currentCodec->m_param.cfColorFormat);
            }
        }

        if (JxrMacroblockRegionStateShouldTransform(&region,
            currentCodec->m_Dparam->bDecodeFullFrame)) {
            JxrDecoderTransformPipelineApply(currentCodec);
            if (currentCodec->cColumn < currentCodec->cmbWidth &&
                currentCodec->cRow < currentCodec->cmbHeight) {
                JXRTraceDumpStage("decoder", "reconstructed_samples", currentCodec,
                    (Int)currentCodec->cColumn, (Int)currentCodec->cRow, JXRTraceOutput);
            }
        }

        if (planeIndex < hasSecondaryCodec) {
            currentCodec->m_pNextSC->cRow = currentCodec->cRow;
            currentCodec->m_pNextSC->cColumn = currentCodec->cColumn;
            currentCodec = currentCodec->m_pNextSC;
        }
    }

    return result;
}
