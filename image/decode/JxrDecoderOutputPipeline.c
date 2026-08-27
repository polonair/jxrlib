#include "JxrDecoderOutputPipeline.h"
#include "decode.h"
#include "JxrInverseColorTransform.h"
#include "JxrSampleClipping.h"
#include "JxrFloatSampleConversion.h"
#include "JxrMonochromeExpansion.h"
#include "JxrDecoderRoiRowRange.h"
#include "JxrDecoderUvInterpolator.h"

Void JxrDecoderOutputPipelinePlanInitialize(JxrDecoderOutputPipelinePlan* plan,
    Bool hasOptimizedLoadOverride)
{
    plan->usesLegacyLoadCallback = hasOptimizedLoadOverride;
}

// write one MB row of Y_ONLY/CF_ALPHA/YUV_444/N_CHANNEL to output buffer
static Void JxrDecoderOutputPipelineWriteNChannel(CWMImageStrCodec * pSC, size_t iFirstRow, size_t iFirstColumn, size_t cWidth, size_t cHeight, size_t iShift, PixelI iBias)
{
    const CWMImageInfo* pII = &pSC->WMII;
    const size_t cChannel = pII->cfColorFormat == Y_ONLY ? 1 : pSC->WMISCP.cChannel;
    // const U8 cbChannels[BDB_MAX] = {-1, 1, 2, 2, 2, 4, 4, -1, -1, };
    const U8 nLen = pSC->WMISCP.nLenMantissaOrShift;
    const I8 nExpBias = pSC->WMISCP.nExpBias;

    PixelI * pChannel[16];
    size_t iChannel, iRow, iColumn;
    size_t * pOffsetX = pSC->m_Dparam->pOffsetX, * pOffsetY = pSC->m_Dparam->pOffsetY + (pSC->cRow - 1) * 16, iY;

    assert(cChannel <= 16);

    for(iChannel = 0; iChannel < cChannel; iChannel ++)
        pChannel[iChannel & 15] = pSC->a0MBbuffer[iChannel];

    if(pSC->m_bUVResolutionChange)
        pChannel[1] = pSC->pResU, pChannel[2] = pSC->pResV;

    switch(pSC->WMII.bdBitDepth){
        case BD_8:
            for(iRow = iFirstRow; iRow < cHeight; iRow ++){
                for(iColumn = iFirstColumn, iY = pOffsetY[iRow]; iColumn < cWidth; iColumn ++){
                    U8 * pDst = (U8 *)pSC->WMIBI.pv + iY + pOffsetX[iColumn];

                    for(iChannel = 0; iChannel < cChannel; iChannel ++){
                        PixelI p = ((pChannel[iChannel & 15][((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 15]] + iBias) >> iShift);

                        pDst[iChannel] = JxrSampleClippingToByte(p);
                    }
                }
            }
            break;

        case BD_16:
            for(iRow = iFirstRow; iRow < cHeight; iRow ++){
                for(iColumn = iFirstColumn, iY = pOffsetY[iRow]; iColumn < cWidth; iColumn ++){
                    U16 * pDst = (U16 *)pSC->WMIBI.pv + iY + pOffsetX[iColumn];

                    for(iChannel = 0; iChannel < cChannel; iChannel ++){
                        PixelI p = ((pChannel[iChannel & 15][((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 15]] + iBias) >> iShift);

                        p <<= nLen;
                        pDst[iChannel] = JxrSampleClippingToUInt16(p);
                    }
                }
            }
            break;

        case BD_16S:
            for(iRow = iFirstRow; iRow < cHeight; iRow ++){
                for(iColumn = iFirstColumn, iY = pOffsetY[iRow]; iColumn < cWidth; iColumn ++){
                    I16 * pDst = (I16 *)pSC->WMIBI.pv + iY + pOffsetX[iColumn];

                    for(iChannel = 0; iChannel < cChannel; iChannel ++){
                        PixelI p = ((pChannel[iChannel & 15][((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 15]] + iBias) >> iShift);

                        p <<= nLen;
                        pDst[iChannel] = JxrSampleClippingToInt16(p);
                    }
                }
            }
            break;

        case BD_16F:
            for(iRow = iFirstRow; iRow < cHeight; iRow ++){
                for(iColumn = iFirstColumn, iY = pOffsetY[iRow]; iColumn < cWidth; iColumn ++){
                    U16 * pDst = (U16 *)pSC->WMIBI.pv + iY + pOffsetX[iColumn];

                    for(iChannel = 0; iChannel < cChannel; iChannel ++){
                        PixelI p = ((pChannel[iChannel & 15][((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf]] + iBias) >> iShift);

                        pDst[iChannel] = JxrFloatSampleConversionToHalf(p);
                    }
                }
            }
            break;

        case BD_32:
            for(iRow = iFirstRow; iRow < cHeight; iRow ++){
                for(iColumn = iFirstColumn, iY = pOffsetY[iRow]; iColumn < cWidth; iColumn ++){
                    U32 * pDst = (U32 *)pSC->WMIBI.pv + iY + pOffsetX[iColumn];

                    for(iChannel = 0; iChannel < cChannel; iChannel ++){
                        PixelI p = ((pChannel[iChannel & 15][((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf]] + iBias) >> iShift);

                        p <<= nLen;
                        pDst[iChannel] = (U32)(p);
                    }
                }
            }
            break;

        case BD_32S:
            for(iRow = iFirstRow; iRow < cHeight; iRow ++){
                for(iColumn = iFirstColumn, iY = pOffsetY[iRow]; iColumn < cWidth; iColumn ++){
                    I32 * pDst = (I32 *)pSC->WMIBI.pv + iY + pOffsetX[iColumn];

                    for(iChannel = 0; iChannel < cChannel; iChannel ++){
                        PixelI p = ((pChannel[iChannel & 15][((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf]] + iBias) >> iShift);

                        p <<= nLen;
                        pDst[iChannel] = (I32)(p);
                    }
                }
            }
            break;

        case BD_32F:
            for(iRow = iFirstRow; iRow < cHeight; iRow ++){
                for(iColumn = iFirstColumn, iY = pOffsetY[iRow]; iColumn < cWidth; iColumn ++){
                    float * pDst = (float *)pSC->WMIBI.pv + iY + pOffsetX[iColumn];

                    for(iChannel = 0; iChannel < cChannel; iChannel ++){
                        PixelI p = ((pChannel[iChannel & 15][((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf]] + iBias) >> iShift);

                        pDst[iChannel] = JxrFloatSampleConversionToSingle(p, nExpBias, nLen);
                    }
                }
            }
            break;

        default:
            assert(0);
            break;
    }
}

// centralized alpha channel color conversion, small perf penalty
static Int JxrDecoderOutputPipelineWriteAlphaRow(CWMImageStrCodec * pSC)
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

Int JxrDecoderOutputPipelineWriteStandardRow(CWMImageStrCodec * pSC)
{
    const COLORFORMAT cfExt = (pSC->m_param.cfColorFormat == Y_ONLY ? Y_ONLY : pSC->WMII.cfColorFormat);
    const BITDEPTH_BITS bd = pSC->WMII.bdBitDepth;
    const PixelI iShift = (pSC->m_param.bScaledArith ? SHIFTZERO + QPFRACBITS : 0);
    const size_t cHeight = JxrDecoderRoiRowRangeGetOutputHeight(pSC->m_Dparam->cROIBottomY + 1, pSC->cRow);
    const size_t cWidth = (pSC->m_Dparam->cROIRightX + 1);
    const size_t iFirstRow = ((pSC->cRow - 1) * 16 > pSC->m_Dparam->cROITopY ? 0 : (pSC->m_Dparam->cROITopY & 0xf)), iFirstColumn = pSC->m_Dparam->cROILeftX;
    const PixelI *pY = pSC->a0MBbuffer[0];
    const PixelI *pU = (pSC->m_bUVResolutionChange ? pSC->pResU : pSC->a0MBbuffer[1]);
    const PixelI *pV = (pSC->m_bUVResolutionChange ? pSC->pResV : pSC->a0MBbuffer[2]);
    const PixelI *pA = NULL;
	const size_t iB = (pSC->WMII.bRGB ? 2 : 0);
	const size_t iR = 2 - iB;
    const U8 nLen = pSC->WMISCP.nLenMantissaOrShift;
    const I8 nExpBias = pSC->WMISCP.nExpBias;
    size_t iRow, iColumn, iIdx;
    size_t * pOffsetX = pSC->m_Dparam->pOffsetX, * pOffsetY = pSC->m_Dparam->pOffsetY + (pSC->cRow - 1) * (cfExt == YUV_420 ? 8 : 16), iY;


    if (pSC->m_pNextSC) {
        assert (pSC->m_param.bScaledArith == pSC->m_pNextSC->m_param.bScaledArith);  // will be relaxed later
    }

    // guard output buffer
    if(checkImageBuffer(pSC, pSC->WMII.oOrientation >= O_RCW ? pSC->WMII.cROIHeight : pSC->WMII.cROIWidth, cHeight - iFirstRow) != ICERR_OK)
        return ICERR_ERROR;

    if(pSC->m_bUVResolutionChange)
        JxrDecoderUvInterpolatorInterpolate(pSC);

    if(pSC->WMISCP.bYUVData){
        I32 * pDst = (I32 *)pSC->WMIBI.pv + (pSC->cRow - 1) *
            (pSC->m_param.cfColorFormat == YUV_420 ? 8 : 16) * pSC->WMIBI.cbStride / sizeof(I32);

        switch(pSC->m_param.cfColorFormat){
        case Y_ONLY:
        case YUV_444:
        case NCOMPONENT:
            {
                PixelI * pChannel[16];
                size_t iChannel;

                const size_t cChannel = pSC->WMII.cfColorFormat == Y_ONLY ? 1 : pSC->WMISCP.cChannel;
                assert(cChannel <= 16);

                for(iChannel = 0; iChannel < cChannel; iChannel ++)
                    pChannel[iChannel & 15] = pSC->a0MBbuffer[iChannel];

                for(iRow = iFirstRow; iRow < cHeight; iRow ++){
                    I32 * pRow = pDst;
                    for(iColumn = iFirstColumn; iColumn < cWidth; iColumn ++){
                        for(iChannel = 0; iChannel < cChannel; iChannel ++){
                            PixelI p = pChannel[iChannel & 15][((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf]];

                            *pRow++ = p;
                        }
                    }
                    pDst += pSC->WMIBI.cbStride / sizeof(I32);
                }
            }
            break;

        case YUV_422:
            {
				PixelI y0, y1, u, v;

				for(iRow = iFirstRow; iRow < cHeight; iRow ++){
                    I32 * pRow = pDst;
					for(iColumn = iFirstColumn; iColumn < cWidth; iColumn += 2){
						iIdx = ((iColumn >> 4) << 7) + idxCC[iRow][(iColumn >> 1) & 7];
						u = pU[iIdx], v = pV[iIdx];

						y0 = pY[((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 15]];
						y1 = pY[(((iColumn + 1) >> 4) << 8) + idxCC[iRow][(iColumn + 1) & 15]];

						pRow[0] = u, pRow[1] = y0, pRow[2] = v, pRow[3] = y1;
                        pRow += 4;
					}
                    pDst += pSC->WMIBI.cbStride / sizeof(I32);
				}
			}
            break;

        case YUV_420:
			{
				PixelI y0, y1, y2, y3, u, v;
				// const size_t iS4[8][4] = {{0, 1, 2, 3}, {2, 3, 0, 1}, {1, 0, 3, 2}, {3, 2, 1, 0}, {1, 3, 0, 2}, {3, 1, 2, 0}, {0, 2, 1, 3}, {2, 0, 3, 1}};

				for(iRow = iFirstRow; iRow < cHeight; iRow += 2){
                    I32 * pRow = pDst;
					for(iColumn = iFirstColumn; iColumn < cWidth; iColumn += 2){
						iIdx = ((iColumn >> 4) << 6) + idxCC_420[iRow >> 1][(iColumn >> 1) & 7];
						u = pU[iIdx], v = pV[iIdx];

						y0 = pY[((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 15]];
						y1 = pY[(((iColumn + 1) >> 4) << 8) + idxCC[iRow][(iColumn + 1) & 15]];
						y2 = pY[((iColumn >> 4) << 8) + idxCC[iRow + 1][iColumn & 15]];
						y3 = pY[(((iColumn + 1) >> 4) << 8) + idxCC[iRow + 1][(iColumn + 1) & 15]];

						pRow[0] = y0, pRow[1] = y1, pRow[2] = y2, pRow[3] = y3, pRow[4] = u, pRow[5] = v;
                        pRow += 6;
					}
                    pDst += pSC->WMIBI.cbStride / sizeof(I32);
				}
			}
            break;

        default:
            assert(0);
            break;
        }
    }
    else if(bd == BD_8){
        U8 * pDst;
        const PixelI iBias1 = 128 << iShift;
        const PixelI iBias2 = pSC->m_param.bScaledArith ? ((1 << (SHIFTZERO + QPFRACBITS - 1)) - 1) : 0;
        const PixelI iBias = iBias1 + iBias2;

        switch(cfExt){
        case CF_RGB:
        {
            PixelI r, g, b, a;

            if (pSC->m_pNextSC && pSC->WMISCP.uAlphaMode > 0) { // RGBA

                pA = pSC->m_pNextSC->a0MBbuffer[0];

                if (pSC->m_param.bScaledArith == FALSE) {
                    for(iRow = iFirstRow; iRow < cHeight; iRow ++)
                        for(iColumn = iFirstColumn, iY = pOffsetY[iRow]; iColumn < cWidth; iColumn ++){
                            iIdx = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 15];

                            g = pY[iIdx] + iBias, r = -pU[iIdx], b = pV[iIdx];
                            a = pA[iIdx] + iBias;

                            JxrInverseColorTransformApplyRgb(&r, &g, &b);

                            pDst = (U8 *)pSC->WMIBI.pv + pOffsetX[iColumn] + iY;
                            if ((g | b | r | a) & ~0xff)
                                pDst[iR] = JxrSampleClippingToByte(r), pDst[1] = JxrSampleClippingToByte(g), pDst[iB] = JxrSampleClippingToByte(b), pDst[3] = JxrSampleClippingToByte(a);
                            else
                                pDst[iR] = (U8)r, pDst[1] = (U8)g, pDst[iB] = (U8)b, pDst[3] = (U8)(a);
                        }
                }
                else{
                    for(iRow = iFirstRow; iRow < cHeight; iRow ++)
                        for(iColumn = iFirstColumn, iY = pOffsetY[iRow]; iColumn < cWidth; iColumn ++){
                            iIdx = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 15];

                            g = pY[iIdx] + iBias, r = -pU[iIdx], b = pV[iIdx];
                            a = pA[iIdx] + iBias;

                            JxrInverseColorTransformApplyRgb(&r, &g, &b);

                            g >>= iShift, b >>= iShift, r >>= iShift, a >>= iShift;
                            pDst = (U8 *)pSC->WMIBI.pv + pOffsetX[iColumn] + iY;
                            if ((g | b | r | a) & ~0xff)
                                pDst[iR] = JxrSampleClippingToByte(r), pDst[1] = JxrSampleClippingToByte(g), pDst[iB] = JxrSampleClippingToByte(b), pDst[3] = JxrSampleClippingToByte(a);
                            else
                                pDst[iR] = (U8)r, pDst[1] = (U8)g, pDst[iB] = (U8)b, pDst[3] = (U8)(a);
                        }
                }
            }
            else {
                if (pSC->m_param.bScaledArith == FALSE) {
                    for(iRow = iFirstRow; iRow < cHeight; iRow ++)
                        for(iColumn = iFirstColumn, iY = pOffsetY[iRow]; iColumn < cWidth; iColumn ++){
                            iIdx = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 15];

                            g = pY[iIdx] + iBias, r = -pU[iIdx], b = pV[iIdx];

                            JxrInverseColorTransformApplyRgb(&r, &g, &b);

                            pDst = (U8 *)pSC->WMIBI.pv + pOffsetX[iColumn] + iY;
                            if ((g | b | r) & ~0xff)
                                pDst[iR] = JxrSampleClippingToByte(r), pDst[1] = JxrSampleClippingToByte(g), pDst[iB] = JxrSampleClippingToByte(b);
                            else
                                pDst[iR] = (U8)r, pDst[1] = (U8)g, pDst[iB] = (U8)b;
                        }
                }
                else{
                    for(iRow = iFirstRow; iRow < cHeight; iRow ++)
                        for(iColumn = iFirstColumn, iY = pOffsetY[iRow]; iColumn < cWidth; iColumn ++){
                            iIdx = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 15];

                            g = pY[iIdx] + iBias, r = -pU[iIdx], b = pV[iIdx];

                            JxrInverseColorTransformApplyRgb(&r, &g, &b);

                            g >>= iShift, b >>= iShift, r >>= iShift;
                            pDst = (U8 *)pSC->WMIBI.pv + pOffsetX[iColumn] + iY;
                            if ((g | b | r) & ~0xff)
                                pDst[iR] = JxrSampleClippingToByte(r), pDst[1] = JxrSampleClippingToByte(g), pDst[iB] = JxrSampleClippingToByte(b);
                            else
                                pDst[iR] = (U8)r, pDst[1] = (U8)g, pDst[iB] = (U8)b;
                        }
                }
            }
            break;
        }

        case Y_ONLY:
        case YUV_444:
        case NCOMPONENT:
            JxrDecoderOutputPipelineWriteNChannel(pSC, iFirstRow, iFirstColumn, cWidth, cHeight, iShift, iBias);
            break;

        case YUV_422:
			{
				PixelI y0, y1, u, v;
				// const ORIENTATION oO = pSC->WMII.oOrientation;
				// const size_t i0 = ((oO > O_FLIPV && oO <= O_RCW_FLIPVH) ? 1 : 0), i1 = 1 - i0;

				for(iRow = iFirstRow; iRow < cHeight; iRow ++){
					for(iColumn = iFirstColumn, iY = pOffsetY[iRow]; iColumn < cWidth; iColumn += 2){
						iIdx = ((iColumn >> 4) << 7) + idxCC[iRow][(iColumn >> 1) & 7];
						u = ((pU[iIdx] + iBias) >> iShift), v = ((pV[iIdx] + iBias) >> iShift);

						y0 = ((pY[((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 15]] + iBias) >> iShift);
						y1 = ((pY[(((iColumn + 1) >> 4) << 8) + idxCC[iRow][(iColumn + 1) & 15]] + iBias) >> iShift);

						pDst = (U8 *)pSC->WMIBI.pv + pOffsetX[iColumn >> 1] + iY;
						if ((y0 | y1 | u | v) & ~0xff)//UYVY
							pDst[0] = JxrSampleClippingToByte(u), pDst[1] = JxrSampleClippingToByte(y0), pDst[2] = JxrSampleClippingToByte(v), pDst[3] = JxrSampleClippingToByte(y1);
						else
							pDst[0] = (U8)u, pDst[1] = (U8)y0, pDst[2] = (U8)v, pDst[3] = (U8)y1;
					}
				}
			}
			break;

        case YUV_420:
			{
				PixelI y0, y1, y2, y3, u, v;
				const size_t iS4[8][4] = {{0, 1, 2, 3}, {2, 3, 0, 1}, {1, 0, 3, 2}, {3, 2, 1, 0}, {1, 3, 0, 2}, {3, 1, 2, 0}, {0, 2, 1, 3}, {2, 0, 3, 1}};
				const ORIENTATION oO = pSC->WMII.oOrientation;
				const size_t i0 = iS4[oO][0], i1 = iS4[oO][1], i2 = iS4[oO][2], i3 = iS4[oO][3];

				for(iRow = iFirstRow; iRow < cHeight; iRow += 2){
					for(iColumn = iFirstColumn, iY = pOffsetY[iRow >> 1]; iColumn < cWidth; iColumn += 2){
						iIdx = ((iColumn >> 4) << 6) + idxCC_420[iRow >> 1][(iColumn >> 1) & 7];
						u = ((pU[iIdx] + iBias) >> iShift), v = ((pV[iIdx] + iBias) >> iShift);

						y0 = ((pY[((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 15]] + iBias) >> iShift);
						y1 = ((pY[(((iColumn + 1) >> 4) << 8) + idxCC[iRow][(iColumn + 1) & 15]] + iBias) >> iShift);
						y2 = ((pY[((iColumn >> 4) << 8) + idxCC[iRow + 1][iColumn & 15]] + iBias) >> iShift);
						y3 = ((pY[(((iColumn + 1) >> 4) << 8) + idxCC[iRow + 1][(iColumn + 1) & 15]] + iBias) >> iShift);

						pDst = (U8 *)pSC->WMIBI.pv + pOffsetX[iColumn >> 1] + iY;
						if ((y0 | y1 | y2 | y3 | u | v) & ~0xff)
							pDst[i0] = JxrSampleClippingToByte(y0), pDst[i1] = JxrSampleClippingToByte(y1), pDst[i2] = JxrSampleClippingToByte(y2), pDst[i3] = JxrSampleClippingToByte(y3), pDst[4] = JxrSampleClippingToByte(u), pDst[5] = JxrSampleClippingToByte(v);
						else
							pDst[i0] = (U8)y0, pDst[i1] = (U8)y1, pDst[i2] = (U8)y2, pDst[i3] = (U8)y3, pDst[4] = (U8)u, pDst[5] = (U8)v;
					}
				}
			}
			break;

        case CMYK:
			{
				PixelI c, m, y, k;
				PixelI * pK = pSC->a0MBbuffer[3];

				for(iRow = iFirstRow; iRow < cHeight; iRow++){
					for(iColumn = iFirstColumn, iY = pOffsetY[iRow]; iColumn < cWidth; iColumn++){
						iIdx = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 15];

						m = -pY[iIdx] + iBias1, c = pU[iIdx], y = -pV[iIdx], k = pK[iIdx] + iBias2;

						JxrInverseColorTransformApplyCmyk(&c, &m, &y, &k); // color conversion

						c >>= iShift, m >>= iShift, y >>= iShift, k >>= iShift;

						pDst = (U8 *)pSC->WMIBI.pv + pOffsetX[iColumn] + iY;
						if ((c | m | y | k) & ~0xff)
							pDst[0] = JxrSampleClippingToByte(c), pDst[1] = JxrSampleClippingToByte(m), pDst[2] = JxrSampleClippingToByte(y), pDst[3] = JxrSampleClippingToByte(k);
						else
							pDst[0] = (U8)c, pDst[1] = (U8)m, pDst[2] = (U8)y, pDst[3] = (U8)k;
					}
				}
			}
			break;

        case CF_RGBE:
			{
				PixelI r, g, b;

				for(iRow = iFirstRow; iRow < cHeight; iRow ++){
					for(iColumn = iFirstColumn, iY = pOffsetY[iRow]; iColumn < cWidth; iColumn ++){
							iIdx = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 15];

							g = pY[iIdx] + iBias2, r = -pU[iIdx], b = pV[iIdx];

							JxrInverseColorTransformApplyRgb(&r, &g, &b);

							pDst = (U8 *)pSC->WMIBI.pv + pOffsetX[iColumn] + iY;

							{
                                JxrRgbeSample rgbe = JxrFloatSampleConversionToRgbe(r >> iShift, g >> iShift, b >> iShift);
                                pDst[0] = rgbe.red;
                                pDst[1] = rgbe.green;
                                pDst[2] = rgbe.blue;
                                pDst[3] = rgbe.exponent;
                            }
						}
				}
			}
			break;

        default:
            assert(0);
            break;
    }
    }
    else if(bd == BD_16){
        const PixelI iBias = (((1 << 15) >> nLen) << iShift) + (iShift == 0 ? 0 : (1 << (iShift - 1)));
        U16 * pDst;

        switch(cfExt){
        case CF_RGB:
        {
            PixelI r, g, b;
			if (pSC->m_param.bScaledArith == FALSE) {
				for(iRow = iFirstRow; iRow < cHeight; iRow ++)
					for(iColumn = iFirstColumn, iY = pOffsetY[iRow]; iColumn < cWidth; iColumn ++){
						iIdx = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 15];

						g = pY[iIdx] + iBias, r = -pU[iIdx], b = pV[iIdx];

						JxrInverseColorTransformApplyRgb(&r, &g, &b);

						g <<= nLen, b <<= nLen, r <<= nLen;

						pDst = (U16 *)pSC->WMIBI.pv + pOffsetX[iColumn] + iY;

						if ((g | b | r) & ~0xffff)
							pDst[0] = JxrSampleClippingToUInt16(r),  pDst[1] = JxrSampleClippingToUInt16(g), pDst[2] = JxrSampleClippingToUInt16(b);
						else
							pDst[0] = (U16)r, pDst[1] = (U16)g, pDst[2] = (U16)b;
					}
			}
			else{
				for(iRow = iFirstRow; iRow < cHeight; iRow ++)
					for(iColumn = iFirstColumn, iY = pOffsetY[iRow]; iColumn < cWidth; iColumn ++){
						iIdx = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 15];

						g = pY[iIdx] + iBias, r = -pU[iIdx], b = pV[iIdx];

						JxrInverseColorTransformApplyRgb(&r, &g, &b);

						g = (g >> iShift) << nLen, b = (b >> iShift) << nLen, r = (r >> iShift) << nLen;
						pDst = (U16 *)pSC->WMIBI.pv + pOffsetX[iColumn] + iY;
						if ((g | b | r) & ~0xffff)
							pDst[0] = JxrSampleClippingToUInt16(r),  pDst[1] = JxrSampleClippingToUInt16(g), pDst[2] = JxrSampleClippingToUInt16(b);
						else
							pDst[0] = (U16)r, pDst[1] = (U16)g, pDst[2] = (U16)b;
					}
			}
            break;
        }

        case Y_ONLY:
        case YUV_444:
        case NCOMPONENT:
            JxrDecoderOutputPipelineWriteNChannel(pSC, iFirstRow, iFirstColumn, cWidth, cHeight, iShift, iBias);
            break;

        case YUV_422:
			{
				PixelI y0, y1, u, v;
				const ORIENTATION oO = pSC->WMII.oOrientation;
				const size_t i0 = ((oO == O_FLIPH || oO == O_FLIPVH || oO == O_RCW_FLIPV || oO == O_RCW_FLIPVH) ? 1 : 0), i1 = 1 - i0;

				for(iRow = iFirstRow; iRow < cHeight; iRow ++){
					for(iColumn = iFirstColumn, iY = pOffsetY[iRow]; iColumn < cWidth; iColumn += 2){
						iIdx = ((iColumn >> 4) << 7) + idxCC[iRow][(iColumn >> 1) & 7];
						u = ((pU[iIdx] + iBias) >> iShift) << nLen, v = ((pV[iIdx] + iBias) >> iShift) << nLen;

						y0 = ((pY[((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 15]] + iBias) >> iShift) << nLen;
						y1 = ((pY[(((iColumn + 1) >> 4) << 8) + idxCC[iRow][(iColumn + 1) & 15]] + iBias) >> iShift) << nLen;

						pDst = (U16 *)pSC->WMIBI.pv + pOffsetX[iColumn >> 1] + iY;
						if ((y0 | y1 | u | v) & ~0xffff)
							{
								pDst[i0] = JxrSampleClippingToUInt16(u);
								pDst[i1] = JxrSampleClippingToUInt16(y0);
								pDst[2] = JxrSampleClippingToUInt16(v);
								pDst[3] = JxrSampleClippingToUInt16(y1);
							}
						else
							{
								pDst[i0] = (U16)(u);
								pDst[i1] = (U16)(y0);
								pDst[2] = (U16)(v);
								pDst[3] = (U16)(y1);
							}
					}
				}
			}
			break;

        case YUV_420:
			{
				PixelI y0, y1, y2, y3, u, v;
				const size_t iS4[8][4] = {{0, 1, 2, 3}, {2, 3, 0, 1}, {1, 0, 3, 2}, {3, 2, 1, 0}, {1, 3, 0, 2}, {3, 1, 2, 0}, {0, 2, 1, 3}, {2, 0, 3, 1}};
				const ORIENTATION oO = pSC->WMII.oOrientation;
				const size_t i0 = iS4[oO][0], i1 = iS4[oO][1], i2 = iS4[oO][2], i3 = iS4[oO][3];

				for(iRow = iFirstRow; iRow < cHeight; iRow += 2){
					for(iColumn = iFirstColumn, iY = pOffsetY[iRow >> 1]; iColumn < cWidth; iColumn += 2){
						iIdx = ((iColumn >> 3) << 6) + idxCC[iRow][(iColumn >> 1) & 7];
						u = ((pU[iIdx] + iBias) >> iShift) << nLen, v = ((pV[iIdx] + iBias) >> iShift) << nLen;

						y0 = ((pY[((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 15]] + iBias) >> iShift) << nLen;
						y1 = ((pY[(((iColumn + 1) >> 4) << 8) + idxCC[iRow][(iColumn + 1) & 15]] + iBias) >> iShift) << nLen;
						y2 = ((pY[((iColumn >> 4) << 8) + idxCC[iRow + 1][iColumn & 15]] + iBias) >> iShift) << nLen;
						y3 = ((pY[(((iColumn + 1) >> 4) << 8) + idxCC[iRow + 1][(iColumn + 1) & 15]] + iBias) >> iShift) << nLen;

						pDst = (U16 *)pSC->WMIBI.pv + pOffsetX[iColumn >> 1] + iY;
						if ((y0 | y1 | y2 | y3 | u | v) & ~0xffff)
							{
								pDst[i0] = JxrSampleClippingToUInt16(y0);
								pDst[i1] = JxrSampleClippingToUInt16(y1);
								pDst[i2] = JxrSampleClippingToUInt16(y2);
								pDst[i3] = JxrSampleClippingToUInt16(y3);
								pDst[4] = JxrSampleClippingToUInt16(u);
								pDst[5] = JxrSampleClippingToUInt16(v);
							}
						else
							{
								pDst[i0] = (U16)(y0);
								pDst[i1] = (U16)(y1);
								pDst[i2] = (U16)(y2);
								pDst[i3] = (U16)(y3);
								pDst[4] = (U16)(u);
								pDst[5] = (U16)(v);
							}
					}
				}
			}
			break;

        case CMYK:
			{
				PixelI c, m, y, k;
				PixelI * pK = pSC->a0MBbuffer[3];
				const PixelI iBias1 = (32768 >> nLen) << iShift;
				const PixelI iBias2 = iBias - iBias1;

				for(iRow = iFirstRow; iRow < cHeight; iRow++){
					for(iColumn = iFirstColumn, iY = pOffsetY[iRow]; iColumn < cWidth; iColumn++){
						iIdx = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 15];

						m = -pY[iIdx] + iBias1, c = pU[iIdx], y = -pV[iIdx], k = pK[iIdx] + iBias2;

						JxrInverseColorTransformApplyCmyk(&c, &m, &y, &k); // color conversion

						c = (c >> iShift) << nLen, m = (m >> iShift) << nLen, y = (y >> iShift) << nLen, k = (k >> iShift) << nLen;

						pDst = (U16 *)pSC->WMIBI.pv + pOffsetX[iColumn] + iY;
						if ((c | m | y | k) & ~0xffff)
							pDst[0] = JxrSampleClippingToUInt16(c), pDst[1] = JxrSampleClippingToUInt16(m), pDst[2] = JxrSampleClippingToUInt16(y), pDst[3] = JxrSampleClippingToUInt16(k);
						else
							pDst[0] = (U16)(c), pDst[1] = (U16)(m), pDst[2] = (U16)(y), pDst[3] = (U16)(k);
						}
				}
			}
			break;
        default:
            assert(0);
            break;
    }
    }
    else if(bd == BD_16S){
        const PixelI iBias = pSC->m_param.bScaledArith ? ((1 << (SHIFTZERO + QPFRACBITS - 1)) - 1) : 0;
        I16 * pDst;

        switch(cfExt){
        case CF_RGB:
        {
            PixelI r, g, b;

            for(iRow = iFirstRow; iRow < cHeight; iRow ++)
                for(iColumn = iFirstColumn, iY = pOffsetY[iRow]; iColumn < cWidth; iColumn ++){
                    iIdx = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 15];

                    g = pY[iIdx] + iBias, r = -pU[iIdx], b = pV[iIdx];

                    JxrInverseColorTransformApplyRgb(&r, &g, &b);

                    r = (r >> iShift) << nLen, g = (g >> iShift) << nLen, b = (b >> iShift) << nLen;

                    pDst = (I16 *)pSC->WMIBI.pv + pOffsetX[iColumn] + iY;
                    pDst[0] = JxrSampleClippingToInt16(r), pDst[1] = JxrSampleClippingToInt16(g), pDst[2] = JxrSampleClippingToInt16(b);
                }
            break;
        }

        case Y_ONLY:
        case YUV_444:
        case NCOMPONENT:
            JxrDecoderOutputPipelineWriteNChannel(pSC, iFirstRow, iFirstColumn, cWidth, cHeight, iShift, iBias);
            break;

        case CMYK:
			{
				PixelI c, m, y, k;
				PixelI * pK = pSC->a0MBbuffer[3];

				for(iRow = iFirstRow; iRow < cHeight; iRow++){
					for(iColumn = iFirstColumn, iY = pOffsetY[iRow]; iColumn < cWidth; iColumn++){
						iIdx = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 15];

						m = -pY[iIdx], c = pU[iIdx], y = -pV[iIdx], k = pK[iIdx] + iBias;

						JxrInverseColorTransformApplyCmyk(&c, &m, &y, &k); // color conversion

						c = (c >> iShift) << nLen, m = (m >> iShift) << nLen, y = (y >> iShift) << nLen, k = (k >> iShift) << nLen;

						pDst = (I16 *)pSC->WMIBI.pv + pOffsetX[iColumn] + iY;
						pDst[0] = (I16)(c), pDst[1] = (I16)(m), pDst[2] = (I16)(y), pDst[3] = (I16)(k);
					}
				}
			}
			break;

        default:
            assert(0);
            break;
    }
    }
    else if(bd == BD_16F){
        const PixelI iBias = pSC->m_param.bScaledArith ? ((1 << (SHIFTZERO + QPFRACBITS - 1)) - 1) : 0;
        U16 *pDst;

        switch (cfExt)
        {
        case CF_RGB:
        {
            PixelI r, g, b;

            for(iRow = iFirstRow; iRow < cHeight; iRow ++){
                for(iColumn = iFirstColumn, iY = pOffsetY[iRow]; iColumn < cWidth; iColumn ++){
                    iIdx = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 15];

                    g = pY[iIdx] + iBias, r = -pU[iIdx], b = pV[iIdx];

                    JxrInverseColorTransformApplyRgb(&r, &g, &b);

                    pDst = (U16 *)pSC->WMIBI.pv + pOffsetX[iColumn] + iY;
                    pDst[0] = JxrFloatSampleConversionToHalf(r >> iShift);
                    pDst[1] = JxrFloatSampleConversionToHalf(g >> iShift);
                    pDst[2] = JxrFloatSampleConversionToHalf(b >> iShift);
                }
            }
            break;
        }

        case Y_ONLY:
        case YUV_444:
        case NCOMPONENT:
            JxrDecoderOutputPipelineWriteNChannel(pSC, iFirstRow, iFirstColumn, cWidth, cHeight, iShift, iBias);
            break;

        default:
            assert(0);
            break;
        }
    }
    else if(bd == BD_32){
        const PixelI iBias = (((1 << 31) >> nLen) << iShift) + (iShift == 0 ? 0 : (1 << (iShift - 1)));
        U32 * pDst;

        switch (cfExt)
        {
        case CF_RGB:
			{
				PixelI r, g, b;

				for(iRow = iFirstRow; iRow < cHeight; iRow ++){
					for(iColumn = iFirstColumn, iY = pOffsetY[iRow]; iColumn < cWidth; iColumn ++){
						iIdx = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 15];

						g = pY[iIdx] + iBias, r = -pU[iIdx], b = pV[iIdx];

						JxrInverseColorTransformApplyRgb(&r, &g, &b);

						pDst = (U32 *)pSC->WMIBI.pv + pOffsetX[iColumn] + iY;
						pDst[0] = ((r >> iShift) << nLen);
						pDst[1] = ((g >> iShift) << nLen);
						pDst[2] = ((b >> iShift) << nLen);
					}
				}
			}
			break;

        case Y_ONLY:
        case YUV_444:
        case NCOMPONENT:
        {
            JxrDecoderOutputPipelineWriteNChannel(pSC, iFirstRow, iFirstColumn, cWidth, cHeight, iShift, iBias);
            break;
        }
        default:
            assert(0);
            break;
        }
    }
    else if(bd == BD_32S){
        const PixelI iBias = pSC->m_param.bScaledArith ? ((1 << (SHIFTZERO + QPFRACBITS - 1)) - 1) : 0;
        int * pDst;

        switch (cfExt)
        {
        case CF_RGB:
        {
            PixelI r, g, b;

            for(iRow = iFirstRow; iRow < cHeight; iRow ++){
                for(iColumn = iFirstColumn, iY = pOffsetY[iRow]; iColumn < cWidth; iColumn ++){
                    iIdx = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 15];

                    g = pY[iIdx] + iBias, r = -pU[iIdx], b = pV[iIdx];

                    JxrInverseColorTransformApplyRgb(&r, &g, &b);

                    pDst = (int *)pSC->WMIBI.pv + pOffsetX[iColumn] + iY;
                    pDst[0] = ((r >> iShift) << nLen);
                    pDst[1] = ((g >> iShift) << nLen);
                    pDst[2] = ((b >> iShift) << nLen);
                }
            }
            break;
        }

        case Y_ONLY:
        case YUV_444:
        case NCOMPONENT:
            JxrDecoderOutputPipelineWriteNChannel(pSC, iFirstRow, iFirstColumn, cWidth, cHeight, iShift, iBias);
            break;

		default:
            assert(0);
            break;
        }
    }
    else if(bd == BD_32F){
        const PixelI iBias = pSC->m_param.bScaledArith ? ((1 << (SHIFTZERO + QPFRACBITS - 1)) - 1) : 0;
        float * pDst;

        switch (cfExt)
        {
        case CF_RGB:
        {
            PixelI r, g, b;

            for(iRow = iFirstRow; iRow < cHeight; iRow ++){
                for(iColumn = iFirstColumn, iY = pOffsetY[iRow]; iColumn < cWidth; iColumn ++){
                    iIdx = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 15];

                    g = pY[iIdx] + iBias, r = -pU[iIdx], b = pV[iIdx];

                    JxrInverseColorTransformApplyRgb(&r, &g, &b);

                    pDst = (float *)pSC->WMIBI.pv + pOffsetX[iColumn] + iY;
                    pDst[0] = JxrFloatSampleConversionToSingle(r >> iShift, nExpBias, nLen);
                    pDst[1] = JxrFloatSampleConversionToSingle(g >> iShift, nExpBias, nLen);
                    pDst[2] = JxrFloatSampleConversionToSingle(b >> iShift, nExpBias, nLen);
                }
            }
            break;
        }
        case Y_ONLY:
        case YUV_444:
        case NCOMPONENT:
            JxrDecoderOutputPipelineWriteNChannel(pSC, iFirstRow, iFirstColumn, cWidth, cHeight, iShift, iBias);
            break;

        default:
            assert(0);
            break;
        }
    }
    else if(bd == BD_5){
        const PixelI iBias = (16 << iShift) + (pSC->m_param.bScaledArith ? ((1 << (SHIFTZERO + QPFRACBITS - 1)) - 1) : 0);
        PixelI r, g, b;
        U16 * pDst;

        assert(cfExt == CF_RGB);

        for(iRow = iFirstRow; iRow < cHeight; iRow ++)
            for(iColumn = iFirstColumn, iY = pOffsetY[iRow]; iColumn < cWidth; iColumn ++){
                iIdx = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 15];

                g = pY[iIdx] + iBias, r = -pU[iIdx], b = pV[iIdx];

                JxrInverseColorTransformApplyRgb(&r, &g, &b);

                g >>= iShift, b >>= iShift, r >>= iShift;
                pDst = (U16 *)pSC->WMIBI.pv + pOffsetX[iColumn] + iY;
                if (pSC->m_param.bRBSwapped)
                    pDst[0] = (U16)JxrSampleClippingClamp(b, 0, 31) + (((U16)JxrSampleClippingClamp(g, 0, 31)) << 5) + (((U16)JxrSampleClippingClamp(r, 0, 31)) << 10);
                else
                    pDst[0] = (U16)JxrSampleClippingClamp(r, 0, 31) + (((U16)JxrSampleClippingClamp(g, 0, 31)) << 5) + (((U16)JxrSampleClippingClamp(b, 0, 31)) << 10);
            }
    }
    else if(bd == BD_565){
        const PixelI iBias = (32 << iShift) + (pSC->m_param.bScaledArith ? ((1 << (SHIFTZERO + QPFRACBITS - 1)) - 1) : 0);
        PixelI r, g, b;
        U16 * pDst;

        assert(cfExt == CF_RGB);

        for(iRow = iFirstRow; iRow < cHeight; iRow ++)
            for(iColumn = iFirstColumn, iY = pOffsetY[iRow]; iColumn < cWidth; iColumn ++){
                iIdx = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 15];

                g = pY[iIdx] + iBias, r = -pU[iIdx], b = pV[iIdx];

                JxrInverseColorTransformApplyRgb(&r, &g, &b);

                g >>= iShift, b >>= iShift + 1, r >>= iShift + 1;
                pDst = (U16 *)pSC->WMIBI.pv + pOffsetX[iColumn] + iY;
                if (pSC->m_param.bRBSwapped)
                    pDst[0] = (U16)JxrSampleClippingClamp(b, 0, 31) + (((U16)JxrSampleClippingClamp(g, 0, 63)) << 5) + (((U16)JxrSampleClippingClamp(r, 0, 31)) << 11);
                else
                    pDst[0] = (U16)JxrSampleClippingClamp(r, 0, 31) + (((U16)JxrSampleClippingClamp(g, 0, 63)) << 5) + (((U16)JxrSampleClippingClamp(b, 0, 31)) << 11);
            }
    }
    else if(bd == BD_10){
        const PixelI iBias = (512 << iShift) + (pSC->m_param.bScaledArith ? ((1 << (SHIFTZERO + QPFRACBITS - 1)) - 1) : 0);
        PixelI r, g, b;
        U32 * pDst;

        assert(cfExt == CF_RGB);

        for(iRow = iFirstRow; iRow < cHeight; iRow ++)
            for(iColumn = iFirstColumn, iY = pOffsetY[iRow]; iColumn < cWidth; iColumn ++){
                iIdx = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 15];

                g = pY[iIdx] + iBias, r = -pU[iIdx], b = pV[iIdx];

                JxrInverseColorTransformApplyRgb(&r, &g, &b);

                g >>= iShift, b >>= iShift, r >>= iShift;

                pDst = (U32 *)pSC->WMIBI.pv + pOffsetX[iColumn] + iY;
                if (pSC->m_param.bRBSwapped)
                    pDst[0] = (U32)JxrSampleClippingClamp(b, 0, 1023) +
                        (((U32)JxrSampleClippingClamp(g, 0, 1023)) << 10) +
                        (((U32)JxrSampleClippingClamp(r, 0, 1023)) << 20);
                else
                    pDst[0] = (U32)JxrSampleClippingClamp(r, 0, 1023) +
                        (((U32)JxrSampleClippingClamp(g, 0, 1023)) << 10) +
                        (((U32)JxrSampleClippingClamp(b, 0, 1023)) << 20);
            }
    }
    else if(bd == BD_1){
        const size_t iPos = pSC->WMII.cLeadingPadding;
        const Int iTh = (iShift > 0) ? (1 << (iShift - 1)) : 1;
        assert(cfExt == Y_ONLY && pSC->m_param.cfColorFormat == Y_ONLY);

        if(pSC->WMII.oOrientation < O_RCW)
            for(iRow = iFirstRow; iRow < cHeight; iRow ++) {
                iY = pOffsetY[iRow] + iPos;
                for(iColumn = iFirstColumn; iColumn < cWidth; iColumn ++) {
                    U8 cByte = ((U8 *)pSC->WMIBI.pv + (pOffsetX[iColumn] >> 3) + iY)[0];
                    U8 cShift = (U8)(7 - (pOffsetX[iColumn] & 7));
                    ((U8 *)pSC->WMIBI.pv + (pOffsetX[iColumn] >> 3) + iY)[0] ^= // exor is used because we can't assume the byte was originally zero
                    (((pSC->WMISCP.bBlackWhite + (pY[((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 15]] >= iTh)
                    + (cByte >> cShift)) & 0x1) << cShift);
                }
            }
        else
            for(iRow = iFirstRow; iRow < cHeight; iRow ++) {
                iY = pOffsetY[iRow] + iPos;
                for(iColumn = iFirstColumn; iColumn < cWidth; iColumn ++) {
                    U8 cByte = ((U8 *)pSC->WMIBI.pv + pOffsetX[iColumn] + (iY >> 3))[0];
                    U8 cShift = (U8)(7 - (iY & 7));  // should be optimized out
                    ((U8 *)pSC->WMIBI.pv + pOffsetX[iColumn] + (iY >> 3))[0] ^= // exor is used because we can't assume the byte was originally zero
                    (((pSC->WMISCP.bBlackWhite + (pY[((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 15]] >= iTh)
                    + (cByte >> cShift)) & 0x1) << cShift);
                }
            }
    }

    if(pSC->WMISCP.uAlphaMode > 0)
        if(JxrDecoderOutputPipelineWriteAlphaRow(pSC) != ICERR_OK)
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

// Y_ONLY/CF_ALPHA/YUV_444/N_CHANNEL thumbnail decode
static Void JxrDecoderOutputPipelineWriteNChannelThumbnail(CWMImageStrCodec * pSC, const PixelI cMul, const size_t rShiftY, size_t iFirstRow, size_t iFirstColumn)
{
    const size_t tScale = pSC->m_Dparam->cThumbnailScale;
    const size_t cWidth = (pSC->m_Dparam->cROIRightX + 1);
    const size_t cHeight = JxrDecoderRoiRowRangeGetOutputHeight(pSC->m_Dparam->cROIBottomY + 1, pSC->cRow);
    const size_t cChannel = pSC->WMISCP.cChannel;
    const U8 nLen = pSC->WMISCP.nLenMantissaOrShift;
    const I8 nExpBias = pSC->WMISCP.nExpBias;
    size_t nBits = 0;
    PixelI iOffset;
    PixelI * pChannel[16];
    size_t iChannel, iRow, iColumn;
    size_t * pOffsetX = pSC->m_Dparam->pOffsetX, * pOffsetY = pSC->m_Dparam->pOffsetY + (pSC->cRow - 1) * 16 / tScale, iY;

    while((size_t)(1U << nBits) < tScale)
        nBits ++;

    assert(cChannel <= 16);

    for(iChannel = 0; iChannel < cChannel; iChannel ++)
        pChannel[iChannel & 15] = pSC->a0MBbuffer[iChannel];

    if(pSC->m_bUVResolutionChange)
        pChannel[1] = pSC->pResU, pChannel[2] = pSC->pResV;

    switch(pSC->WMII.bdBitDepth){
        case BD_8:
            for(iOffset = (128 << rShiftY) / cMul, iRow = iFirstRow; iRow < cHeight; iRow += tScale){
                for(iColumn = iFirstColumn, iY = pOffsetY[iRow >> nBits]; iColumn < cWidth; iColumn += tScale){
                    U8 * pDst = (U8 *)pSC->WMIBI.pv + iY + pOffsetX[iColumn >> nBits];

                    for(iChannel = 0; iChannel < cChannel; iChannel ++){
                        PixelI p = ((pChannel[iChannel & 15][((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 15]] + iOffset) * cMul) >> rShiftY;

                        pDst[iChannel] = JxrSampleClippingToByte(p);
                    }
                }
            }
            break;

        case BD_16:
            for(iOffset = (32768 << rShiftY) / cMul, iRow = iFirstRow; iRow < cHeight; iRow += tScale){
                for(iColumn = iFirstColumn, iY = pOffsetY[iRow >> nBits]; iColumn < cWidth; iColumn += tScale){
                    U16 * pDst = (U16 *)pSC->WMIBI.pv + iY + pOffsetX[iColumn >> nBits];

                    for(iChannel = 0; iChannel < cChannel; iChannel ++){
                        PixelI p = (((pChannel[iChannel & 15][((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 15]] + iOffset) * cMul) >> rShiftY) << nLen;

                        pDst[iChannel] = JxrSampleClippingToUInt16(p);
                    }
                }
            }
            break;

        case BD_16S:
            for(iRow = iFirstRow; iRow < cHeight; iRow += tScale){
                for(iColumn = iFirstColumn, iY = pOffsetY[iRow >> nBits]; iColumn < cWidth; iColumn += tScale){
                    I16 * pDst = (I16 *)pSC->WMIBI.pv + iY + pOffsetX[iColumn >> nBits];

                    for(iChannel = 0; iChannel < cChannel; iChannel ++){
                        PixelI p = ((pChannel[iChannel & 15][((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 15]] * cMul) >> rShiftY) << nLen;

                        pDst[iChannel] = JxrSampleClippingToInt16(p);
                    }
                }
            }
            break;

        case BD_16F:
            for(iRow = iFirstRow; iRow < cHeight; iRow += tScale){
                for(iColumn = iFirstColumn, iY = pOffsetY[iRow >> nBits]; iColumn < cWidth; iColumn += tScale){
                    U16 * pDst = (U16 *)pSC->WMIBI.pv + iY + pOffsetX[iColumn >> nBits];

                    for(iChannel = 0; iChannel < cChannel; iChannel ++){
                        PixelI p = (pChannel[iChannel & 15][((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 15]] * cMul) >> rShiftY;

                        pDst[iChannel] = JxrFloatSampleConversionToHalf(p);
                    }
                }
            }
            break;
        case BD_32:
            for(iOffset = (((1 << 31) >> nLen) << rShiftY) / cMul, iRow = iFirstRow; iRow < cHeight; iRow += tScale){
                for(iColumn = iFirstColumn, iY = pOffsetY[iRow >> nBits]; iColumn < cWidth; iColumn += tScale){
                    U32 * pDst = (U32 *)pSC->WMIBI.pv + iY + pOffsetX[iColumn >> nBits];

                    for(iChannel = 0; iChannel < cChannel; iChannel ++){
                        PixelI p = (((pChannel[iChannel & 15][((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 15]] + iOffset) * cMul) >> rShiftY) << nLen;

                        pDst[iChannel] = (U32)(p);
                    }
                }
            }
            break;
        case BD_32S:
            for(iRow = iFirstRow; iRow < cHeight; iRow += tScale){
                for(iColumn = iFirstColumn, iY = pOffsetY[iRow >> nBits]; iColumn < cWidth; iColumn += tScale){
                    I32 * pDst = (I32 *)pSC->WMIBI.pv + iY + pOffsetX[iColumn >> nBits];

                    for(iChannel = 0; iChannel < cChannel; iChannel ++){
                        PixelI p = ((pChannel[iChannel & 15][((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 15]] * cMul) >> rShiftY) << nLen;

                        pDst[iChannel] = (I32)(p);
                    }
                }
            }
            break;
        case BD_32F:
            for(iRow = iFirstRow; iRow < cHeight; iRow += tScale){
                for(iColumn = iFirstColumn, iY = pOffsetY[iRow >> nBits]; iColumn < cWidth; iColumn += tScale){
                    float * pDst = (float *)pSC->WMIBI.pv + iY + pOffsetX[iColumn >> nBits];

                    for(iChannel = 0; iChannel < cChannel; iChannel ++){
                        PixelI p = (pChannel[iChannel & 15][((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 15]] * cMul) >> rShiftY;

                        pDst[iChannel] = JxrFloatSampleConversionToSingle(p, nExpBias, nLen);
                    }
                }
            }
            break;

        default:
            assert(0);
            break;
    }
}

// centralized alpha channel thumbnail, small perf penalty
static Int JxrDecoderOutputPipelineWriteThumbnailAlpha(CWMImageStrCodec * pSC, const size_t nBits, const PixelI cMul, const size_t rShiftY)
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

Int JxrDecoderOutputPipelineWriteThumbnailRow(CWMImageStrCodec * pSC)
{
    const size_t tScale = pSC->m_Dparam->cThumbnailScale;
    const size_t cHeight = JxrDecoderRoiRowRangeGetOutputHeight(pSC->m_Dparam->bDecodeFullFrame ? pSC->WMII.cHeight : pSC->m_Dparam->cROIBottomY + 1, pSC->cRow);
    const size_t cWidth = (pSC->m_Dparam->bDecodeFullFrame ? pSC->WMII.cWidth : pSC->m_Dparam->cROIRightX + 1);
    const size_t iFirstRow = ((((pSC->cRow - 1) * 16 > pSC->m_Dparam->cROITopY ? 0 : (pSC->m_Dparam->cROITopY & 0xf)) + tScale - 1) / tScale * tScale);
    const size_t iFirstColumn = (pSC->m_Dparam->cROILeftX + tScale - 1) / tScale * tScale;
    const COLORFORMAT cfInt = pSC->m_param.cfColorFormat;
    const COLORFORMAT cfExt = (pSC->m_param.cfColorFormat == Y_ONLY ? Y_ONLY : pSC->WMII.cfColorFormat);
    const BITDEPTH_BITS bd = pSC->WMII.bdBitDepth;
    const OVERLAP ol = pSC->WMISCP.olOverlap;
	const size_t iB = (pSC->WMII.bRGB ? 2 : 0);
    const size_t iR = 2 - iB;

    const U8 nLen = pSC->WMISCP.nLenMantissaOrShift;
    const I8 nExpBias = pSC->WMISCP.nExpBias;
    PixelI offset;
    size_t iRow, iColumn, iIdx1, iIdx2, iIdx3 = 0, nBits = 0;
    PixelI * pSrcY = pSC->a0MBbuffer[0];
    PixelI * pSrcU = pSC->a0MBbuffer[1], * pSrcV = pSC->a0MBbuffer[2];
    size_t * pOffsetX = pSC->m_Dparam->pOffsetX, * pOffsetY = pSC->m_Dparam->pOffsetY + (pSC->cRow - 1) * 16 / tScale, iY;
    const PixelI cMul = (tScale >= 16 ? (ol == OL_NONE ? 16 : (ol == OL_ONE ? 23 : 34)) : (tScale >= 4 ? (ol == OL_NONE ? 64 : 93) : 258));
    const size_t rShiftY = 8 + (pSC->m_param.bScaledArith ? (SHIFTZERO + QPFRACBITS) : 0);
    const size_t rShiftUV = rShiftY - ((pSC->m_param.bScaledArith && tScale >= 16) ? ((cfInt == YUV_420 || cfInt == YUV_422) ? 2 : 1) : 0);

    while((size_t)(1U << nBits) < tScale)
        nBits ++;

    assert(tScale == (size_t)(1U << nBits));

    // guard output buffer
    if(checkImageBuffer(pSC, pSC->WMII.oOrientation < O_RCW ? pSC->WMII.cROIWidth : pSC->WMII.cROIHeight, (cHeight - iFirstRow) / pSC->m_Dparam->cThumbnailScale) != ICERR_OK)
        return ICERR_ERROR;

    if((((pSC->cRow - 1) * 16) % tScale) != 0)
        return ICERR_OK;

    if(pSC->cRow * 16 <= pSC->m_Dparam->cROITopY || pSC->cRow * 16 > pSC->m_Dparam->cROIBottomY + 16)
        return ICERR_OK;

    if((cfInt == YUV_422 || cfInt == YUV_420) && cfExt != Y_ONLY){
        PixelI * pDstU = pSC->pResU, * pDstV = pSC->pResV;

        for(iRow = 0; iRow < 16; iRow += tScale){
            for(iColumn = 0; iColumn < cWidth; iColumn += tScale){
                iIdx1 = (cfInt == YUV_422 ? ((iColumn >> 4) << 7) + idxCC[iRow][(iColumn >> 1) & 7] : ((iColumn >> 4) << 6) + idxCC_420[iRow >> 1][(iColumn >> 1) & 7]);
                iIdx2 = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 15];

                // copy over
                pDstU[iIdx2] = pSrcU[iIdx1];
                pDstV[iIdx2] = pSrcV[iIdx1];
            }
        }

        if(tScale == 4){
            if(cfInt == YUV_420){
                for(iColumn = 0; iColumn < cWidth; iColumn += 8){
                    iIdx1 = ((iColumn >> 4) << 8) + idxCC[0][iColumn & 15];
                    iIdx2 = ((iColumn >> 4) << 8) + idxCC[4][iColumn & 15];
                    iIdx3 = ((iColumn >> 4) << 8) + idxCC[8][iColumn & 15];

                    pDstU[iIdx2] = ((pDstU[iIdx1] + pDstU[iIdx3] + 1) >> 1);
                    pDstV[iIdx2] = ((pDstV[iIdx1] + pDstV[iIdx3] + 1) >> 1);

                    iIdx1 = ((iColumn >> 4) << 8) + idxCC[12][iColumn & 15];
                    pDstU[iIdx1] = pDstU[iIdx3];
                    pDstV[iIdx1] = pDstV[iIdx3];
                }
            }

            for(iRow = 0; iRow < 16; iRow += 4){
                for(iColumn = 0; iColumn < cWidth - 8; iColumn += 8){
                    iIdx1 = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 15];
                    iIdx2 = ((iColumn >> 4) << 8) + idxCC[iRow][(iColumn + 4) & 15];
                    iIdx3 = ((iColumn >> 4) << 8) + idxCC[iRow][(iColumn + 8) & 15];

                    pDstU[iIdx2] = ((pDstU[iIdx1] + pDstU[iIdx3] + 1) >> 1);
                    pDstV[iIdx2] = ((pDstV[iIdx1] + pDstV[iIdx3] + 1) >> 1);
                }

                iIdx2 = ((iColumn >> 4) << 8) + idxCC[iRow][(iColumn + 4) & 15];
                pDstU[iIdx2] = pDstU[iIdx3];
                pDstV[iIdx2] = pDstV[iIdx3];
            }
        }

        pSrcU = pDstU, pSrcV = pDstV;
    }

    if(bd == BD_8){
        U8 * pDst;

        offset = (128 << rShiftY) / cMul;

        switch(cfExt){
            case CF_RGB:
                for(iRow = iFirstRow; iRow < cHeight; iRow += tScale){
                    for(iColumn = iFirstColumn, iY = pOffsetY[iRow >> nBits]; iColumn < cWidth; iColumn += tScale){
                        size_t iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
                        PixelI g = ((pSrcY[iPos] + offset) * cMul) >> rShiftY, r = -(pSrcU[iPos] * cMul) >> rShiftUV, b = (pSrcV[iPos] * cMul) >> rShiftUV;

                        JxrInverseColorTransformApplyRgb(&r, &g, &b);

                        pDst = (U8 *)pSC->WMIBI.pv + pOffsetX[iColumn >> nBits] + iY;
                        pDst[iB] = JxrSampleClippingToByte(b), pDst[1] = JxrSampleClippingToByte(g), pDst[iR] = JxrSampleClippingToByte(r);
                }
            }
            break;

        case Y_ONLY:
        case YUV_444:
        case NCOMPONENT:
            JxrDecoderOutputPipelineWriteNChannelThumbnail(pSC, cMul, rShiftY, iFirstRow, iFirstColumn);
            break;

        case CF_RGBE:
            for(iRow = iFirstRow; iRow < cHeight; iRow += tScale){
                for(iColumn = iFirstColumn, iY = pOffsetY[iRow >> nBits]; iColumn < cWidth; iColumn += tScale){
                    size_t iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
                    PixelI g = ((pSrcY[iPos] * cMul) >> rShiftY), r = - ((pSrcU[iPos] * cMul) >> rShiftUV), b = ((pSrcV[iPos] * cMul) >> rShiftUV);

                    JxrInverseColorTransformApplyRgb(&r, &g, &b);

                    pDst = (U8 *)pSC->WMIBI.pv + pOffsetX[iColumn >> nBits] + iY;
                    {
                        JxrRgbeSample rgbe = JxrFloatSampleConversionToRgbe(r, g, b);
                        pDst[0] = rgbe.red;
                        pDst[1] = rgbe.green;
                        pDst[2] = rgbe.blue;
                        pDst[3] = rgbe.exponent;
                    }
                }
            }
            break;

        case CMYK:
        {
            PixelI * pSrcK = pSC->a0MBbuffer[3];
            PixelI iBias1 = (128 << rShiftY) / cMul, iBias2 = (((128 << rShiftUV) / cMul) >> 1);

            for(iRow = iFirstRow; iRow < cHeight; iRow += tScale){
                for(iColumn = iFirstColumn, iY = pOffsetY[iRow >> nBits]; iColumn < cWidth; iColumn += tScale){
                    size_t iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
                    PixelI m = ((-pSrcY[iPos] + iBias1) * cMul) >> rShiftY, c = (pSrcU[iPos] * cMul) >> rShiftUV, y = -(pSrcV[iPos] * cMul) >> rShiftUV, k = ((pSrcK[iPos] + iBias2) * cMul) >> rShiftUV;

                    JxrInverseColorTransformApplyCmyk(&c, &m, &y, &k);

                    pDst = (U8 *)pSC->WMIBI.pv + pOffsetX[iColumn >> nBits] + iY;
                    pDst[0] = JxrSampleClippingToByte(c), pDst[1] = JxrSampleClippingToByte(m), pDst[2] = JxrSampleClippingToByte(y), pDst[3] = JxrSampleClippingToByte(k);
                }
            }
            break;
        }
        default:
            assert(0);
            break;
        }
    }
    if(bd == BD_16){
        U16 * pDst;

        offset = (((1 << 15) >> nLen) << rShiftY) / cMul;

        switch(cfExt){
            case CF_RGB:
                for(iRow = iFirstRow; iRow < cHeight; iRow += tScale){
                    for(iColumn = iFirstColumn, iY = pOffsetY[iRow >> nBits]; iColumn < cWidth; iColumn += tScale){
                        size_t iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
                        PixelI g = ((pSrcY[iPos] + offset) * cMul) >> rShiftY, r = -(pSrcU[iPos] * cMul) >> rShiftUV, b = (pSrcV[iPos] * cMul) >> rShiftUV;

                        JxrInverseColorTransformApplyRgb(&r, &g, &b);

                        pDst = (U16 *)pSC->WMIBI.pv + pOffsetX[iColumn >> nBits] + iY;
                        r <<= nLen, g <<= nLen, b <<= nLen;
                        pDst[0] = JxrSampleClippingToUInt16(r);
                        pDst[1] = JxrSampleClippingToUInt16(g);
                        pDst[2] = JxrSampleClippingToUInt16(b);
                }
            }
            break;

        case Y_ONLY:
        case YUV_444:
        case NCOMPONENT:
            JxrDecoderOutputPipelineWriteNChannelThumbnail(pSC, cMul, rShiftY, iFirstRow, iFirstColumn);
            break;

        case CMYK:
        {
            PixelI * pSrcK = pSC->a0MBbuffer[3];
            PixelI iBias1 = (32768 << rShiftY) / cMul, iBias2 = (((32768 << rShiftUV) / cMul) >> 1);

            for(iRow = iFirstRow; iRow < cHeight; iRow += tScale){
                for(iColumn = iFirstColumn, iY = pOffsetY[iRow >> nBits]; iColumn < cWidth; iColumn += tScale){
                    size_t iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
                    PixelI m = ((-pSrcY[iPos] + iBias1) * cMul) >> rShiftY, c = (pSrcU[iPos] * cMul) >> rShiftUV, y = -(pSrcV[iPos] * cMul) >> rShiftUV, k = ((pSrcK[iPos] + iBias2) * cMul) >> rShiftUV;

                    JxrInverseColorTransformApplyCmyk(&c, &m, &y, &k);

                    pDst = (U16 *)pSC->WMIBI.pv + pOffsetX[iColumn >> nBits] + iY;
                    c <<= nLen, m <<= nLen, y <<= nLen, k <<= nLen;
                    pDst[0] = JxrSampleClippingToUInt16(c);
                    pDst[1] = JxrSampleClippingToUInt16(m);
                    pDst[2] = JxrSampleClippingToUInt16(y);
                    pDst[3] = JxrSampleClippingToUInt16(k);
                }
            }
            break;
        }
        default:
            assert(0);
            break;
        }
    }
    if(bd == BD_16S){
        I16 * pDst;

        switch(cfExt){
            case CF_RGB:
                for(iRow = iFirstRow; iRow < cHeight; iRow += tScale){
                    for(iColumn = iFirstColumn, iY = pOffsetY[iRow >> nBits]; iColumn < cWidth; iColumn += tScale){
                        size_t iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
                        PixelI g = (pSrcY[iPos] * cMul) >> rShiftY, r = -(pSrcU[iPos] * cMul) >> rShiftUV, b = (pSrcV[iPos] * cMul) >> rShiftUV;

                        JxrInverseColorTransformApplyRgb(&r, &g, &b);

                        pDst = (I16 *)pSC->WMIBI.pv + pOffsetX[iColumn >> nBits] + iY;
                        r <<= nLen, g <<= nLen, b <<= nLen;
                        pDst[0] = JxrSampleClippingToInt16(r);
                        pDst[1] = JxrSampleClippingToInt16(g);
                        pDst[2] = JxrSampleClippingToInt16(b);
                }
            }
            break;

        case Y_ONLY:
        case YUV_444:
        case NCOMPONENT:
            JxrDecoderOutputPipelineWriteNChannelThumbnail(pSC, cMul, rShiftY, iFirstRow, iFirstColumn);
            break;

        case CMYK:
			{
				PixelI * pSrcK = pSC->a0MBbuffer[3];

				for(iRow = iFirstRow; iRow < cHeight; iRow += tScale){
					for(iColumn = iFirstColumn, iY = pOffsetY[iRow >> nBits]; iColumn < cWidth; iColumn += tScale){
						size_t iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
						PixelI m = -(pSrcY[iPos] * cMul) >> rShiftY, c = (pSrcU[iPos] * cMul) >> rShiftUV, y = -(pSrcV[iPos] * cMul) >> rShiftUV, k = (pSrcK[iPos] * cMul) >> rShiftUV;

						JxrInverseColorTransformApplyCmyk(&c, &m, &y, &k);

						pDst = (I16 *)pSC->WMIBI.pv + pOffsetX[iColumn >> nBits] + iY;
						c <<= nLen, m <<= nLen, y <<= nLen, k <<= nLen;
						pDst[0] = JxrSampleClippingToInt16(c);
						pDst[1] = JxrSampleClippingToInt16(m);
						pDst[2] = JxrSampleClippingToInt16(y);
						pDst[3] = JxrSampleClippingToInt16(k);
					}
				}
			}
			break;

        default:
            assert(0);
            break;
        }
    }
    else if(bd == BD_16F){
        U16 * pDst;

        switch(cfExt){
            case CF_RGB:
                for(iRow = iFirstRow; iRow < cHeight; iRow += tScale){
                    for(iColumn = iFirstColumn, iY = pOffsetY[iRow >> nBits]; iColumn < cWidth; iColumn += tScale){
                        size_t iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
                        PixelI g = (pSrcY[iPos] * cMul) >> rShiftY, r = -(pSrcU[iPos] * cMul) >> rShiftUV, b = (pSrcV[iPos] * cMul) >> rShiftUV;

                        JxrInverseColorTransformApplyRgb(&r, &g, &b);

                        pDst = (U16 *)pSC->WMIBI.pv + pOffsetX[iColumn >> nBits] + iY;
                        pDst[0] = JxrFloatSampleConversionToHalf(r);
                        pDst[1] = JxrFloatSampleConversionToHalf(g);
                        pDst[2] = JxrFloatSampleConversionToHalf(b);
                    }
                }
                break;

            case Y_ONLY:
            case YUV_444:
            case NCOMPONENT:
                JxrDecoderOutputPipelineWriteNChannelThumbnail(pSC, cMul, rShiftY, iFirstRow, iFirstColumn);
            break;

            default:
                assert(0);
                break;
        }
    }
    else if(bd == BD_32){
        U32 * pDst;

        offset = (((1 << 31) >> nLen) << rShiftY) / cMul;

        switch(cfExt){
            case CF_RGB:
                for(iRow = iFirstRow; iRow < cHeight; iRow += tScale){
                    for(iColumn = iFirstColumn, iY = pOffsetY[iRow >> nBits]; iColumn < cWidth; iColumn += tScale){
                        size_t iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
                        PixelI g = ((pSrcY[iPos] + offset) * cMul) >> rShiftY, r = -(pSrcU[iPos] * cMul) >> rShiftUV, b = (pSrcV[iPos] * cMul) >> rShiftUV;

                        JxrInverseColorTransformApplyRgb(&r, &g, &b);

                        pDst = (U32 *)pSC->WMIBI.pv + pOffsetX[iColumn >> nBits] + iY;

                        pDst[0] = (U32)(r << nLen);
                        pDst[1] = (U32)(g << nLen);
                        pDst[2] = (U32)(b << nLen);
                    }
                }
                break;

            case Y_ONLY:
            case YUV_444:
            case NCOMPONENT:
                JxrDecoderOutputPipelineWriteNChannelThumbnail(pSC, cMul, rShiftY, iFirstRow, iFirstColumn);
            break;

            default:
                assert(0);
                break;
        }
    }
    else if(bd == BD_32S){
        I32 * pDst;

        switch(cfExt){
            case CF_RGB:
                for(iRow = iFirstRow; iRow < cHeight; iRow += tScale){
                    for(iColumn = iFirstColumn, iY = pOffsetY[iRow >> nBits]; iColumn < cWidth; iColumn += tScale){
                        size_t iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
                        PixelI g = (pSrcY[iPos] * cMul) >> rShiftY, r = -(pSrcU[iPos] * cMul) >> rShiftUV, b = (pSrcV[iPos] * cMul) >> rShiftUV;

                        JxrInverseColorTransformApplyRgb(&r, &g, &b);

                        pDst = (I32 *)pSC->WMIBI.pv + pOffsetX[iColumn >> nBits] + iY;
                        pDst[0] = (I32)(r << nLen);
                        pDst[1] = (I32)(g << nLen);
                        pDst[2] = (I32)(b << nLen);
                    }
                }
                break;

            case Y_ONLY:
            case YUV_444:
            case NCOMPONENT:
                JxrDecoderOutputPipelineWriteNChannelThumbnail(pSC, cMul, rShiftY, iFirstRow, iFirstColumn);
            break;

            default:
                assert(0);
                break;
        }
    }

    else if(bd == BD_32F){
        float * pDst;

        switch(cfExt){
            case CF_RGB:
                for(iRow = iFirstRow; iRow < cHeight; iRow += tScale){
                    for(iColumn = iFirstColumn, iY = pOffsetY[iRow >> nBits]; iColumn < cWidth; iColumn += tScale){
                        size_t iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
                        PixelI g = (pSrcY[iPos] * cMul) >> rShiftY, r = -(pSrcU[iPos] * cMul) >> rShiftUV, b = (pSrcV[iPos] * cMul) >> rShiftUV;

                        JxrInverseColorTransformApplyRgb(&r, &g, &b);

                        pDst = (float *)pSC->WMIBI.pv + pOffsetX[iColumn >> nBits] + iY;
                        pDst[0] = JxrFloatSampleConversionToSingle(r, nExpBias, nLen);
                        pDst[1] = JxrFloatSampleConversionToSingle(g, nExpBias, nLen);
                        pDst[2] = JxrFloatSampleConversionToSingle(b, nExpBias, nLen);
                    }
                }
                break;

            case Y_ONLY:
            case YUV_444:
            case NCOMPONENT:
                JxrDecoderOutputPipelineWriteNChannelThumbnail(pSC, cMul, rShiftY, iFirstRow, iFirstColumn);
                break;

            default:
                assert(0);
                break;
        }
    }
    else if(bd == BD_1){
        const size_t iPos = pSC->WMII.cLeadingPadding;
        Bool bBW;
        U8 cByte, cShift;
        assert(cfExt == Y_ONLY && pSC->m_param.cfColorFormat == Y_ONLY);

        if(pSC->WMII.oOrientation < O_RCW){
            for(iRow = iFirstRow; iRow < cHeight; iRow += tScale)
                for(iColumn = iFirstColumn, iY = pOffsetY[iRow >> nBits] + iPos; iColumn < cWidth; iColumn += tScale){
                    bBW = (pSC->WMISCP.bBlackWhite ^ (pSrcY[((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 15]] > 0));
                    cByte = ((U8 *)pSC->WMIBI.pv + (pOffsetX[iColumn >> nBits] >> 3) + iY)[0];
                    cShift = (U8)(7 - (pOffsetX[iColumn >> nBits] & 7));
                    ((U8 *)pSC->WMIBI.pv + (pOffsetX[iColumn >> nBits] >> 3) + iY)[0] ^= ((((bBW + (cByte >> cShift)) & 0x1)) << cShift);
                }
        }
        else{
            for(iRow = iFirstRow; iRow < cHeight; iRow += tScale)
                for(iColumn = iFirstColumn, iY = pOffsetY[iRow >> nBits] + iPos; iColumn < cWidth; iColumn += tScale){
                    bBW = (pSC->WMISCP.bBlackWhite ^ (pSrcY[((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 15]] > 0));
                    cByte = ((U8 *)pSC->WMIBI.pv + pOffsetX[iColumn >> nBits] + (iY >> 3))[0];
                    cShift = (U8)(7 - (iY & 7));
                    ((U8 *)pSC->WMIBI.pv + pOffsetX[iColumn >> nBits] + (iY >> 3))[0] ^= ((((bBW + (cByte >> cShift)) & 0x1)) << cShift);
                }
        }
    }
    else if(bd == BD_5){
        U16 * pDst;

        offset = (16 << rShiftY) / cMul;

        for(iRow = iFirstRow; iRow < cHeight; iRow += tScale){
            for(iColumn = iFirstColumn, iY = pOffsetY[iRow >> nBits]; iColumn < cWidth; iColumn += tScale){
                size_t iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
                PixelI g = ((pSrcY[iPos] + offset) * cMul) >> rShiftY, r = -(pSrcU[iPos] * cMul) >> rShiftUV, b = (pSrcV[iPos] * cMul) >> rShiftUV;

                JxrInverseColorTransformApplyRgb(&r, &g, &b);
                pDst = (U16 *)pSC->WMIBI.pv + pOffsetX[iColumn >> nBits] + iY;
                pDst[0] = (U16)JxrSampleClippingClamp(r, 0, 31) + (((U16)JxrSampleClippingClamp(g, 0, 31)) << 5) + (((U16)JxrSampleClippingClamp(b, 0, 31)) << 10);
            }
        }
    }
    else if(bd == BD_565){
        U16 * pDst;

        offset = (32 << rShiftY) / cMul;

        for(iRow = iFirstRow; iRow < cHeight; iRow += tScale){
            for(iColumn = iFirstColumn, iY = pOffsetY[iRow >> nBits]; iColumn < cWidth; iColumn += tScale){
                size_t iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
                PixelI g = ((pSrcY[iPos] + offset) * cMul) >> rShiftY, r = -(pSrcU[iPos] * cMul) >> rShiftUV, b = (pSrcV[iPos] * cMul) >> rShiftUV;

                JxrInverseColorTransformApplyRgb(&r, &g, &b);
                r /= 2, b /= 2;
                pDst = (U16 *)pSC->WMIBI.pv + pOffsetX[iColumn >> nBits] + iY;
                pDst[0] = (U16)JxrSampleClippingClamp(r, 0, 31) + (((U16)JxrSampleClippingClamp(g, 0, 63)) << 5) + (((U16)JxrSampleClippingClamp(b, 0, 31)) << 11);
            }
        }
    }
    else if(bd == BD_10){
        U32 * pDst;

        offset = (512 << rShiftY) / cMul;

        for(iRow = iFirstRow; iRow < cHeight; iRow += tScale){
            for(iColumn = iFirstColumn, iY = pOffsetY[iRow >> nBits]; iColumn < cWidth; iColumn += tScale){
                size_t iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
                PixelI g = ((pSrcY[iPos] + offset) * cMul) >> rShiftY, r = -(pSrcU[iPos] * cMul) >> rShiftUV, b = (pSrcV[iPos] * cMul) >> rShiftUV;

                JxrInverseColorTransformApplyRgb(&r, &g, &b);
                pDst = (U32 *)pSC->WMIBI.pv + pOffsetX[iColumn >> nBits] + iY;
                pDst[0] = (U32)JxrSampleClippingClamp(r, 0, 1023) +
                    (((U32)JxrSampleClippingClamp(g, 0, 1023)) << 10) +
                    (((U32)JxrSampleClippingClamp(b, 0, 1023)) << 20);
            }
        }
    }

    if(pSC->WMISCP.uAlphaMode > 0)
        if(JxrDecoderOutputPipelineWriteThumbnailAlpha(pSC, nBits, cMul, rShiftY) != ICERR_OK)
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
