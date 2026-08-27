#include "JxrDecoderAlphaOutputWriter.h"
#include "decode.h"
#include "JxrSampleClipping.h"
#include "JxrFloatSampleConversion.h"
#include "JxrDecoderRoiRowRange.h"
Int JxrDecoderAlphaOutputWriterWrite(CWMImageStrCodec * pSC)
{
    if(pSC->WMII.bdBitDepth == BD_8 && pSC->WMISCP.cfColorFormat == CF_RGB)  // has been taken care of and optimized out
        return ICERR_OK;

    if(pSC->m_bSecondary == FALSE && pSC->m_pNextSC != NULL){ // with alpha channel
        const BITDEPTH_BITS bd = pSC->WMII.bdBitDepth;
        const PixelI iShift = (pSC->m_param.bScaledArith ? SHIFTZERO + QPFRACBITS : 0);
        const size_t cHeight = JxrDecoderRoiRowRangeGetOutputHeight(pSC->m_Dparam->cROIBottomY + 1, pSC->cRow);
        const size_t cWidth = (pSC->m_Dparam->cROIRightX + 1);
        const size_t iFirstRow = ((pSC->cRow - 1) * 16 > pSC->m_Dparam->cROITopY ? 0 : (pSC->m_Dparam->cROITopY & 0xf)), iFirstColumn = pSC->m_Dparam->cROILeftX;
        const size_t iAlphaPos = pSC->WMII.cLeadingPadding + (pSC->WMII.cfColorFormat == CMYK ? 4 : 3);//only RGB and CMYK may have interleaved alpha
        const PixelI * pA = pSC->m_pNextSC->a0MBbuffer[0];
        const U8 nLen = pSC->WMISCP.nLenMantissaOrShift;
        const I8 nExpBias = pSC->WMISCP.nExpBias;
        size_t iRow, iColumn;
        size_t * pOffsetX = pSC->m_Dparam->pOffsetX, * pOffsetY = pSC->m_Dparam->pOffsetY + (pSC->cRow - 1) * 16, iY;

        if (CF_RGB != pSC->WMII.cfColorFormat && CMYK != pSC->WMII.cfColorFormat)
            return ICERR_ERROR;

        if(bd == BD_8){
            const PixelI iBias = (1 << (iShift + 7)) + (iShift == 0 ? 0 : (1 << (iShift - 1)));

            for(iRow = iFirstRow; iRow < cHeight; iRow ++)
                for(iColumn = iFirstColumn, iY = pOffsetY[iRow]; iColumn < cWidth; iColumn ++){
                    PixelI a = ((pA[((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 15]] + iBias) >> iShift);
                    ((U8 *)pSC->WMIBI.pv + pOffsetX[iColumn] + iY)[iAlphaPos] = JxrSampleClippingToByte(a);
                }
        }
        else if(bd == BD_16){
            const PixelI iBias = (1 << (iShift + 15)) + (iShift == 0 ? 0 : (1 << (iShift - 1)));

            for(iRow = iFirstRow; iRow < cHeight; iRow ++)
                for(iColumn = iFirstColumn, iY = pOffsetY[iRow]; iColumn < cWidth; iColumn ++){
                    PixelI a = (((pA[((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 15]] + iBias) >> iShift) << nLen);
                    ((U16 *)pSC->WMIBI.pv + pOffsetX[iColumn] + iY)[iAlphaPos] = JxrSampleClippingToUInt16(a);
                }
        }
        else if(bd == BD_16S){
            const PixelI iBias = (iShift == 0 ? 0 : (1 << (iShift - 1)));

            for(iRow = iFirstRow; iRow < cHeight; iRow ++)
                for(iColumn = iFirstColumn, iY = pOffsetY[iRow]; iColumn < cWidth; iColumn ++){
                    PixelI a = (((pA[((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 15]] + iBias) >> iShift) << nLen);
                    ((I16 *)pSC->WMIBI.pv + pOffsetX[iColumn] + iY)[iAlphaPos] = JxrSampleClippingToInt16(a);
                }
        }
        else if(bd == BD_16F){
            const PixelI iBias = (iShift == 0 ? 0 : (1 << (iShift - 1)));

            for(iRow = iFirstRow; iRow < cHeight; iRow ++)
                for(iColumn = iFirstColumn, iY = pOffsetY[iRow]; iColumn < cWidth; iColumn ++){
                    PixelI a = ((pA[((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 15]] + iBias) >> iShift);
                    ((U16 *)pSC->WMIBI.pv + pOffsetX[iColumn] + iY)[iAlphaPos] = JxrFloatSampleConversionToHalf(a);
                }
        }
        else if(bd == BD_32S){
            const PixelI iBias = (iShift == 0 ? 0 : (1 << (iShift - 1)));

            for(iRow = iFirstRow; iRow < cHeight; iRow ++)
                for(iColumn = iFirstColumn, iY = pOffsetY[iRow]; iColumn < cWidth; iColumn ++){
                    PixelI a = (((pA[((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 15]] + iBias) >> iShift) << nLen);
                    ((I32 *)pSC->WMIBI.pv + pOffsetX[iColumn] + iY)[iAlphaPos] = a;
                }
        }
        else if(bd == BD_32F){
            const PixelI iBias = (iShift == 0 ? 0 : (1 << (iShift - 1)));

            for(iRow = iFirstRow; iRow < cHeight; iRow ++)
                for(iColumn = iFirstColumn, iY = pOffsetY[iRow]; iColumn < cWidth; iColumn ++){
                    PixelI a = ((pA[((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 15]] + iBias) >> iShift);
                    ((float *)pSC->WMIBI.pv + pOffsetX[iColumn] + iY)[iAlphaPos] = JxrFloatSampleConversionToSingle(a, nExpBias, nLen);
                }
        }
        else // not supported
            return ICERR_ERROR;
    }

    return ICERR_OK;
}
