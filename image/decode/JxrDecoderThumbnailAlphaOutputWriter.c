#include "JxrDecoderThumbnailAlphaOutputWriter.h"
#include "decode.h"
#include "JxrSampleClipping.h"
#include "JxrFloatSampleConversion.h"
#include "JxrDecoderRoiRowRange.h"
Int JxrDecoderThumbnailAlphaOutputWriterWrite(CWMImageStrCodec * pSC, const size_t nBits, const PixelI cMul, const size_t rShiftY)
{
    if(pSC->m_bSecondary == FALSE && pSC->m_pNextSC != NULL){ // with alpha channel
        const size_t tScale = (size_t)(1U << nBits);
        const size_t cHeight = JxrDecoderRoiRowRangeGetOutputHeight(pSC->m_Dparam->cROIBottomY + 1, pSC->cRow);
        const size_t cWidth = (pSC->m_Dparam->cROIRightX + 1);
        const size_t iFirstRow = ((((pSC->cRow - 1) * 16 > pSC->m_Dparam->cROITopY ? 0 : (pSC->m_Dparam->cROITopY & 0xf)) + tScale - 1) / tScale * tScale);
        const size_t iFirstColumn = (pSC->m_Dparam->cROILeftX + tScale - 1) / tScale * tScale;
        const size_t iAlphaPos = pSC->WMII.cLeadingPadding + (pSC->WMII.cfColorFormat == CMYK ? 4 : 3);//only RGB and CMYK may have interleaved alpha
        const BITDEPTH_BITS bd = pSC->WMII.bdBitDepth;
        const PixelI * pSrc = pSC->m_pNextSC->a0MBbuffer[0];
        const U8 nLen = pSC->m_pNextSC->WMISCP.nLenMantissaOrShift;
        const I8 nExpBias = pSC->m_pNextSC->WMISCP.nExpBias;
        size_t iRow, iColumn;
        size_t * pOffsetX = pSC->m_Dparam->pOffsetX, * pOffsetY = pSC->m_Dparam->pOffsetY + (pSC->cRow - 1) * 16 / tScale, iY;

        if (CF_RGB != pSC->WMII.cfColorFormat && CMYK != pSC->WMII.cfColorFormat)
            return ICERR_ERROR;

        if(bd == BD_8){
            const PixelI offset = (128 << rShiftY) / cMul;

            for(iRow = iFirstRow; iRow < cHeight; iRow += tScale)
                for(iColumn = iFirstColumn, iY = pOffsetY[iRow >> nBits]; iColumn < cWidth; iColumn += tScale){
                    PixelI a = ((pSrc[((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf]] + offset) * cMul) >> rShiftY;

                    ((U8 *)pSC->WMIBI.pv + pOffsetX[iColumn >> nBits] + iY)[iAlphaPos] = JxrSampleClippingToByte(a);
                }
        }
        else if(bd == BD_16){
            const PixelI offset = (32768 << rShiftY) / cMul;

            for(iRow = iFirstRow; iRow < cHeight; iRow += tScale)
                for(iColumn = iFirstColumn, iY = pOffsetY[iRow >> nBits]; iColumn < cWidth; iColumn += tScale){
                    PixelI a = (((pSrc[((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf]] + offset) * cMul) >> rShiftY) << nLen;

                    ((U16 *)pSC->WMIBI.pv + pOffsetX[iColumn >> nBits] + iY)[iAlphaPos] = JxrSampleClippingToUInt16(a);
                }
        }
        else if(bd == BD_16S){
            for(iRow = iFirstRow; iRow < cHeight; iRow += tScale)
                for(iColumn = iFirstColumn, iY = pOffsetY[iRow >> nBits]; iColumn < cWidth; iColumn += tScale){
                    PixelI a = ((pSrc[((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf]] * cMul) >> rShiftY) << nLen;

                    ((I16 *)pSC->WMIBI.pv + pOffsetX[iColumn >> nBits] + iY)[iAlphaPos] = JxrSampleClippingToInt16(a);
                }
        }
        else if(bd == BD_16F){
            for(iRow = iFirstRow; iRow < cHeight; iRow += tScale)
                for(iColumn = iFirstColumn, iY = pOffsetY[iRow >> nBits]; iColumn < cWidth; iColumn += tScale){
                    PixelI a = (pSrc[((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf]] * cMul) >> rShiftY;

                    ((U16 *)pSC->WMIBI.pv + pOffsetX[iColumn >> nBits] + iY)[iAlphaPos] = JxrFloatSampleConversionToHalf(a);
                }
        }
        else if(bd == BD_32S){
            for(iRow = iFirstRow; iRow < cHeight; iRow += tScale)
                for(iColumn = iFirstColumn, iY = pOffsetY[iRow >> nBits]; iColumn < cWidth; iColumn += tScale){
                    PixelI a = ((pSrc[((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf]] * cMul) >> rShiftY) << nLen;

                    ((I32 *)pSC->WMIBI.pv + pOffsetX[iColumn >> nBits] + iY)[iAlphaPos] = a;
                }
        }
        else if(bd == BD_32F){
            for(iRow = iFirstRow; iRow < cHeight; iRow += tScale)
                for(iColumn = iFirstColumn, iY = pOffsetY[iRow >> nBits]; iColumn < cWidth; iColumn += tScale){
                    PixelI a = (pSrc[((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf]] * cMul) >> rShiftY;

                    ((float *)pSC->WMIBI.pv + pOffsetX[iColumn >> nBits] + iY)[iAlphaPos] = JxrFloatSampleConversionToSingle(a, nExpBias, nLen);
                }
        }
        else // not supported
            return ICERR_ERROR;
    }

    return ICERR_OK;
}
