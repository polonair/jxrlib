#include "JxrDecoderThumbnailColorOutputWriter.h"
#include "JxrDecoderThumbnailNChannelOutputWriter.h"
#include "decode.h"
#include "JxrInverseColorTransform.h"
#include "JxrSampleClipping.h"
#include "JxrFloatSampleConversion.h"

Void JxrDecoderThumbnailColorOutputWriterWrite(CWMImageStrCodec * pSC,
    const JxrDecoderOutputRowPlan * outputPlan,
    const JxrDecoderOutputRowPlan * nChannelPlan,
    const PixelI cMul, const size_t rShiftY, const size_t rShiftUV)
{
    const size_t tScale = outputPlan->thumbnailScale;
    const size_t cHeight = outputPlan->outputHeight;
    const size_t cWidth = outputPlan->outputWidth;
    const size_t iFirstRow = outputPlan->firstRow;
    const size_t iFirstColumn = outputPlan->firstColumn;
    const COLORFORMAT cfInt = outputPlan->internalColorFormat;
    const COLORFORMAT cfExt = outputPlan->outputColorFormat;
    const BITDEPTH_BITS bd = outputPlan->bitDepth;
    const size_t nBits = outputPlan->thumbnailBits;
    const size_t iB = pSC->WMII.bRGB ? 2 : 0;
    const size_t iR = 2 - iB;
    const U8 nLen = pSC->WMISCP.nLenMantissaOrShift;
    const I8 nExpBias = pSC->WMISCP.nExpBias;
    PixelI offset;
    size_t iRow, iColumn, iIdx1, iIdx2, iIdx3 = 0, iY;
    PixelI * pSrcY = pSC->a0MBbuffer[0];
    PixelI * pSrcU = pSC->a0MBbuffer[1];
    PixelI * pSrcV = pSC->a0MBbuffer[2];
    size_t * pOffsetX = pSC->m_Dparam->pOffsetX;
    size_t * pOffsetY = pSC->m_Dparam->pOffsetY +
        (pSC->cRow - 1) * 16 / tScale;
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
            JxrDecoderThumbnailNChannelOutputWriterWrite(pSC, nChannelPlan, cMul, rShiftY);
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
            JxrDecoderThumbnailNChannelOutputWriterWrite(pSC, nChannelPlan, cMul, rShiftY);
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
            JxrDecoderThumbnailNChannelOutputWriterWrite(pSC, nChannelPlan, cMul, rShiftY);
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
                JxrDecoderThumbnailNChannelOutputWriterWrite(pSC, nChannelPlan, cMul, rShiftY);
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
                JxrDecoderThumbnailNChannelOutputWriterWrite(pSC, nChannelPlan, cMul, rShiftY);
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
                JxrDecoderThumbnailNChannelOutputWriterWrite(pSC, nChannelPlan, cMul, rShiftY);
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
                JxrDecoderThumbnailNChannelOutputWriterWrite(pSC, nChannelPlan, cMul, rShiftY);
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
}
