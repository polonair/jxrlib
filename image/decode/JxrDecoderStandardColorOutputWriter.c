#include "JxrDecoderStandardColorOutputWriter.h"
#include "JxrDecoderNChannelOutputWriter.h"
#include "decode.h"
#include "JxrInverseColorTransform.h"
#include "JxrSampleClipping.h"
#include "JxrFloatSampleConversion.h"

Void JxrDecoderStandardColorOutputWriterWrite(CWMImageStrCodec * pSC,
    const JxrDecoderOutputRowPlan * plan, const size_t iShift)
{
    const COLORFORMAT cfExt = plan->outputColorFormat;
    const BITDEPTH_BITS bd = plan->bitDepth;
    const size_t cHeight = plan->outputHeight;
    const size_t cWidth = plan->outputWidth;
    const size_t iFirstRow = plan->firstRow;
    const size_t iFirstColumn = plan->firstColumn;
    const PixelI * pY = pSC->a0MBbuffer[0];
    const PixelI * pU = pSC->m_bUVResolutionChange ? pSC->pResU : pSC->a0MBbuffer[1];
    const PixelI * pV = pSC->m_bUVResolutionChange ? pSC->pResV : pSC->a0MBbuffer[2];
    const PixelI * pA = NULL;
    const size_t iB = pSC->WMII.bRGB ? 2 : 0;
    const size_t iR = 2 - iB;
    const U8 nLen = pSC->WMISCP.nLenMantissaOrShift;
    const I8 nExpBias = pSC->WMISCP.nExpBias;
    size_t iRow, iColumn, iIdx, iY;
    size_t * pOffsetX = pSC->m_Dparam->pOffsetX;
    size_t * pOffsetY = pSC->m_Dparam->pOffsetY +
        (pSC->cRow - 1) * (cfExt == YUV_420 ? 8 : 16);

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
            JxrDecoderNChannelOutputWriterWrite(pSC, iFirstRow, iFirstColumn, cWidth, cHeight, iShift, iBias);
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
            JxrDecoderNChannelOutputWriterWrite(pSC, iFirstRow, iFirstColumn, cWidth, cHeight, iShift, iBias);
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
            JxrDecoderNChannelOutputWriterWrite(pSC, iFirstRow, iFirstColumn, cWidth, cHeight, iShift, iBias);
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
            JxrDecoderNChannelOutputWriterWrite(pSC, iFirstRow, iFirstColumn, cWidth, cHeight, iShift, iBias);
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
            JxrDecoderNChannelOutputWriterWrite(pSC, iFirstRow, iFirstColumn, cWidth, cHeight, iShift, iBias);
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
            JxrDecoderNChannelOutputWriterWrite(pSC, iFirstRow, iFirstColumn, cWidth, cHeight, iShift, iBias);
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
            JxrDecoderNChannelOutputWriterWrite(pSC, iFirstRow, iFirstColumn, cWidth, cHeight, iShift, iBias);
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
}
