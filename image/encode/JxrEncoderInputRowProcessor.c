#include "JxrEncoderInputRowProcessor.h"
#include "JxrEncoderAlphaPlaneInput.h"
#include "JxrEncoderChromaDownsampler.h"
#include "JxrEncoderColorTransform.h"
#include "JxrEncoderInputPadding.h"
#include "JxrEncoderSampleConversion.h"

Void JxrEncoderInputRowPlanInitialize(JxrEncoderInputRowPlan* plan,
    Bool changesUvResolution, U8 alphaMode)
{
    plan->performsChromaDownsampling = changesUvResolution;
    plan->readsAlphaPlane = alphaMode == 3;
}
Int JxrEncoderInputRowProcessorProcess(CWMImageStrCodec* pSC)
{
    const size_t cShift = (pSC->m_param.bScaledArith ? (SHIFTZERO + QPFRACBITS) : 0);
    const BITDEPTH_BITS bdExt = pSC->WMII.bdBitDepth;
    COLORFORMAT cfExt = pSC->WMII.cfColorFormat;
    const COLORFORMAT cfInt = pSC->m_param.cfColorFormat;
    const size_t cPixelStride = (pSC->WMII.cBitsPerUnit >> 3);
    const size_t iRowStride =
		(cfExt == YUV_420 || (pSC->WMISCP.bYUVData && pSC->m_param.cfColorFormat==YUV_420)) ? 2 : 1;
    const size_t cRow = pSC->WMIBI.cLine;
    const size_t cColumn = pSC->WMII.cWidth;
	const size_t iB = (pSC->WMII.bRGB ? 2 : 0);
    const size_t iR = 2 - iB;
    const U8 * pSrc0 = (U8 *)pSC->WMIBI.pv;
    const U8 nLen = pSC->WMISCP.nLenMantissaOrShift;
    const I8 nExpBias = pSC->WMISCP.nExpBias;

    PixelI *pY = pSC->p1MBbuffer[0], *pU = pSC->p1MBbuffer[1], *pV = pSC->p1MBbuffer[2];
    size_t iRow, iColumn, iPos;

    // guard input buffer
    if(checkImageBuffer(pSC, cColumn, cRow) != ICERR_OK)
        return ICERR_ERROR;

    if(pSC->m_bUVResolutionChange)  // will do downsampling somewhere else!
        pU = pSC->pResU, pV = pSC->pResV;
    else if(cfInt == Y_ONLY) // xxx to Y_ONLY transcoding!
        pU = pV = pY; // write pY AFTER pU and pV so Y will overwrite U&V

    for(iRow = 0; iRow < 16; iRow += iRowStride){
        if (pSC->WMISCP.bYUVData){
            I32 * pSrc = (I32 *)pSrc0 + pSC->WMII.cLeadingPadding;

            switch(pSC->m_param.cfColorFormat){
            case Y_ONLY:
            case YUV_444:
            case NCOMPONENT:
                {
                    const size_t cChannel = pSC->m_param.cNumChannels;
                    PixelI * pChannel[16];
                    size_t iChannel;

                    assert(cChannel <= 16);
                    for(iChannel = 0; iChannel < cChannel; iChannel ++)
                        pChannel[iChannel & 15] = pSC->p1MBbuffer[iChannel & 15];
                    if(pSC->m_bUVResolutionChange)
                        pChannel[1] = pSC->pResU, pChannel[2] = pSC->pResV;

                    for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cChannel){
                        iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
                        for(iChannel = 0; iChannel < cChannel; iChannel ++)
                            pChannel[iChannel & 15][iPos] = (PixelI)pSrc[iChannel & 15];
                    }
                }
                break;

            case YUV_422:
                for(iColumn = 0; iColumn < cColumn; iColumn += 2, pSrc += 4){
                    if(cfInt != Y_ONLY){
                        iPos = ((iColumn >> 4) << 7) + idxCC[iRow][(iColumn >> 1) & 7];
                        pU[iPos] = (PixelI)pSrc[0];
                        pV[iPos] = (PixelI)pSrc[2];
                    }

                    pY[((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 15]] = (PixelI)pSrc[1];
                    pY[(((iColumn + 1) >> 4) << 8) + idxCC[iRow][(iColumn + 1) & 15]] = (PixelI)pSrc[3];
                }
                break;

            case YUV_420:
                for(iColumn = 0; iColumn < cColumn; iColumn += 2, pSrc += 6){
                    if(cfInt != Y_ONLY){
                        iPos = ((iColumn >> 4) << 6) + idxCC_420[iRow >> 1][(iColumn >> 1) & 7];
                        pU[iPos] = (PixelI)pSrc[4];
                        pV[iPos] = (PixelI)pSrc[5];
                    }

                    pY[((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 15]] = (PixelI)pSrc[0];
                    pY[(((iColumn + 1) >> 4) << 8) + idxCC[iRow][(iColumn + 1) & 15]] = (PixelI)pSrc[1];
                    pY[((iColumn >> 4) << 8) + idxCC[iRow + 1][iColumn & 15]] = (PixelI)pSrc[2];
                    pY[(((iColumn + 1) >> 4) << 8) + idxCC[iRow + 1][(iColumn + 1) & 15]] = (PixelI)pSrc[3];
                }
                break;

            default:
                assert(0);
                break;
            }
        }
        else if(bdExt == BD_8){
            const U8 * pSrc = pSrc0 + pSC->WMII.cLeadingPadding;
            const PixelI iOffset = (128 << cShift);

            switch(cfExt){
                case CF_RGB:
                    assert (pSC->m_bSecondary == FALSE);
					for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cPixelStride){
						PixelI r = ((PixelI)pSrc[iR]) << cShift, g = ((PixelI)pSrc[1]) << cShift, b = ((PixelI)pSrc[iB]) << cShift;

						JxrEncoderColorTransformApplyRgb(&r, &g, &b); // color conversion

						iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
						pU[iPos] = -r, pV[iPos] = b, pY[iPos] = g - iOffset;
					}
                    break;

                case Y_ONLY:
                case YUV_444:
                case NCOMPONENT:
                {
                    const size_t cChannel = pSC->m_param.cNumChannels;
                    PixelI * pChannel[16];
                    size_t iChannel;

                    assert(cChannel <= 16);
                    for(iChannel = 0; iChannel < cChannel; iChannel ++)
                        pChannel[iChannel & 15] = pSC->p1MBbuffer[iChannel & 15];
                    if(pSC->m_bUVResolutionChange)
                        pChannel[1] = pSC->pResU, pChannel[2] = pSC->pResV;

                    for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cPixelStride){
                        iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
                        for(iChannel = 0; iChannel < cChannel; iChannel ++)
                            pChannel[iChannel & 15][iPos] = (((PixelI)pSrc[iChannel & 15]) << cShift) - iOffset;
                    }
                    break;
                }

                case CF_RGBE:
                    for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cPixelStride){
                        PixelI iExp = (PixelI)pSrc[3];
                        PixelI r = JxrEncoderSampleConversionFromRgbe(pSrc[0], iExp) << cShift;
                        PixelI g = JxrEncoderSampleConversionFromRgbe(pSrc[1], iExp) << cShift;
                        PixelI b = JxrEncoderSampleConversionFromRgbe(pSrc[2], iExp) << cShift;

                        JxrEncoderColorTransformApplyRgb(&r, &g, &b);

                        iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
                        pU[iPos] = -r, pV[iPos] = b, pY[iPos] = g;
                    }
                    break;

                case CMYK:
                {
                    PixelI * pK = (cfInt == CMYK ? pSC->p1MBbuffer[3] : pY); // CMYK -> YUV_xxx transcoding!

                    for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cPixelStride){
                        PixelI c = ((PixelI)pSrc[0]) << cShift;
                        PixelI m = ((PixelI)pSrc[1]) << cShift;
                        PixelI y = ((PixelI)pSrc[2]) << cShift;
                        PixelI k = ((PixelI)pSrc[3]) << cShift;

                        JxrEncoderColorTransformApplyCmyk(&c, &m, &y, &k);

                        iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
                        pU[iPos] = c, pV[iPos] = -y, pK[iPos] = k, pY[iPos] = iOffset - m;
                    }
                    break;
                }

                case YUV_422:
                    for(iColumn = 0; iColumn < cColumn; iColumn += 2, pSrc += cPixelStride){
                        if(cfInt != Y_ONLY){
                            iPos = ((iColumn >> 4) << 7) + idxCC[iRow][(iColumn >> 1) & 7];
                            pU[iPos] = (((PixelI)pSrc[0]) << cShift) - iOffset;
                            pV[iPos] = (((PixelI)pSrc[2]) << cShift) - iOffset;
                        }

                        pY[((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 15]] = (((PixelI)pSrc[1]) << cShift) - iOffset;
                        pY[(((iColumn + 1) >> 4) << 8) + idxCC[iRow][(iColumn + 1) & 15]] = (((PixelI)pSrc[3]) << cShift) - iOffset;
                    }
                    break;

                case YUV_420:
                    for(iColumn = 0; iColumn < cColumn; iColumn += 2, pSrc += cPixelStride){
                        if(cfInt != Y_ONLY){
                            iPos = ((iColumn >> 4) << 6) + idxCC_420[iRow >> 1][(iColumn >> 1) & 7];
                            pU[iPos] = (((PixelI)pSrc[4]) << cShift) - iOffset;
                            pV[iPos] = (((PixelI)pSrc[5]) << cShift) - iOffset;
                        }

                        pY[((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 15]] = (((PixelI)pSrc[0]) << cShift) - iOffset;
                        pY[(((iColumn + 1) >> 4) << 8) + idxCC[iRow][(iColumn + 1) & 15]] = (((PixelI)pSrc[1]) << cShift) - iOffset;
                        pY[((iColumn >> 4) << 8) + idxCC[iRow + 1][iColumn & 15]] = (((PixelI)pSrc[2]) << cShift) - iOffset;
                        pY[(((iColumn + 1) >> 4) << 8) + idxCC[iRow + 1][(iColumn + 1) & 15]] = (((PixelI)pSrc[3]) << cShift) - iOffset;
                    }
                    break;

                default:
                    assert(0);
                    break;
            }
        }
        else if(bdExt == BD_16){
            const U16 * pSrc = (U16 *)pSrc0 + pSC->WMII.cLeadingPadding;
            const size_t cStride = cPixelStride / sizeof(U16);
            const PixelI iOffset = ((1 << 15) >> nLen) << cShift;

            switch(cfExt){
                case CF_RGB:
                    for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cStride){
                        PixelI r = ((PixelI)pSrc[0] >> nLen) << cShift, g = ((PixelI)pSrc[1] >> nLen) << cShift, b = ((PixelI)pSrc[2] >> nLen) << cShift;

                        JxrEncoderColorTransformApplyRgb(&r, &g, &b); // color conversion

                        iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
                        pU[iPos] = -r, pV[iPos] = b, pY[iPos] = g - iOffset;
                    }
                    break;

                case Y_ONLY:
                case YUV_444:
                case NCOMPONENT:
                {
                    const size_t cChannel = pSC->WMISCP.cChannel;
                    size_t iChannel;

                    for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cStride){
                        iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
                        for(iChannel = 0; iChannel < cChannel; iChannel ++)
                            pSC->p1MBbuffer[iChannel][iPos] = (((PixelI)pSrc[iChannel] >> nLen) << cShift) - iOffset;
                    }
                    break;
                }

                case CMYK:
                {
                    PixelI * pK = (cfInt == CMYK ? pSC->p1MBbuffer[3] : pY); // CMYK -> YUV_xxx transcoding!

                    for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cStride){
                        PixelI c = ((PixelI)pSrc[0] >> nLen) << cShift;
                        PixelI m = ((PixelI)pSrc[1] >> nLen) << cShift;
                        PixelI y = ((PixelI)pSrc[2] >> nLen) << cShift;
                        PixelI k = ((PixelI)pSrc[3] >> nLen) << cShift;

                        JxrEncoderColorTransformApplyCmyk(&c, &m, &y, &k);

                        iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
                        pU[iPos] = c, pV[iPos] = -y, pK[iPos] = k, pY[iPos] = iOffset - m;
                    }
                    break;
                }

                case YUV_422:
                    for(iColumn = 0; iColumn < cColumn; iColumn += 2, pSrc += cStride){
                        if(cfInt != Y_ONLY){
                            iPos = ((iColumn >> 4) << 7) + idxCC[iRow][(iColumn >> 1) & 7];
                            pU[iPos] = (((PixelI)pSrc[0]) << cShift) - iOffset;
                            pV[iPos] = (((PixelI)pSrc[2]) << cShift) - iOffset;
                        }

                        pY[((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 15]] = (((PixelI)pSrc[1]) << cShift) - iOffset;
                        pY[(((iColumn + 1) >> 4) << 8) + idxCC[iRow][(iColumn + 1) & 15]] = (((PixelI)pSrc[3]) << cShift) - iOffset;
                    }
                    break;

                case YUV_420:
                    for(iColumn = 0; iColumn < cColumn; iColumn += 2, pSrc += cStride){
                        if(cfInt != Y_ONLY){
                            iPos = ((iColumn >> 4) << 6) + idxCC_420[iRow >> 1][(iColumn >> 1) & 7];
                            pU[iPos] = (((PixelI)pSrc[4]) << cShift) - iOffset;
                            pV[iPos] = (((PixelI)pSrc[5]) << cShift) - iOffset;
                        }

                        pY[((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 15]] = (((PixelI)pSrc[0]) << cShift) - iOffset;
                        pY[(((iColumn + 1) >> 4) << 8) + idxCC[iRow][(iColumn + 1) & 15]] = (((PixelI)pSrc[1]) << cShift) - iOffset;
                        pY[((iColumn >> 4) << 8) + idxCC[iRow + 1][iColumn & 15]] = (((PixelI)pSrc[2]) << cShift) - iOffset;
                        pY[(((iColumn + 1) >> 4) << 8) + idxCC[iRow + 1][(iColumn + 1) & 15]] = (((PixelI)pSrc[3]) << cShift) - iOffset;
                    }
                    break;

                default:
                    assert(0);
                    break;
            }
        }
        else if(bdExt == BD_16S){
            const I16 * pSrc = (I16 *)pSrc0 + pSC->WMII.cLeadingPadding;
            const size_t cStride = cPixelStride / sizeof(I16);

            switch(cfExt){
                case CF_RGB:
                    for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cStride){
                        PixelI r = ((PixelI)pSrc[0] >> nLen) << cShift, g = ((PixelI)pSrc[1] >> nLen) << cShift, b = ((PixelI)pSrc[2] >> nLen) << cShift;

                        JxrEncoderColorTransformApplyRgb(&r, &g, &b); // color conversion

                        iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
                        pU[iPos] = -r, pV[iPos] = b, pY[iPos] = g;
                    }
                    break;

                case Y_ONLY:
                case YUV_444:
                case NCOMPONENT:
					{
						const size_t cChannel = pSC->WMISCP.cChannel;
						size_t iChannel;

						for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cStride){
							iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
							for(iChannel = 0; iChannel < cChannel; iChannel ++)
								pSC->p1MBbuffer[iChannel][iPos] = (((PixelI)pSrc[iChannel] >> nLen) << cShift);
						}
					}
				    break;

                case CMYK:
					{
						PixelI * pK = (cfInt == CMYK ? pSC->p1MBbuffer[3] : pY); // CMYK -> YUV_xxx transcoding!

						for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cStride){
							PixelI c = ((PixelI)pSrc[0] >> nLen) << cShift;
							PixelI m = ((PixelI)pSrc[1] >> nLen) << cShift;
							PixelI y = ((PixelI)pSrc[2] >> nLen) << cShift;
							PixelI k = ((PixelI)pSrc[3] >> nLen) << cShift;

							JxrEncoderColorTransformApplyCmyk(&c, &m, &y, &k);

							iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
							pU[iPos] = c, pV[iPos] = -y, pK[iPos] = k, pY[iPos] = -m;
						}
					}
					break;

                default:
                    assert(0);
                    break;
            }
        }
        else if(bdExt == BD_16F){
            const I16 * pSrc = (I16 *)pSrc0 + pSC->WMII.cLeadingPadding;
            const size_t cStride = cPixelStride / sizeof(U16);

            switch(cfExt){
                case CF_RGB:
                    for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cStride){
                        PixelI r = JxrEncoderSampleConversionFromHalf(pSrc[0]) << cShift;
                        PixelI g = JxrEncoderSampleConversionFromHalf(pSrc[1]) << cShift;
                        PixelI b = JxrEncoderSampleConversionFromHalf(pSrc[2]) << cShift;

                        JxrEncoderColorTransformApplyRgb(&r, &g, &b); // color conversion

                        iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
                        pU[iPos] = -r, pV[iPos] = b, pY[iPos] = g;
                    }
                    break;

                case Y_ONLY:
                case YUV_444:
                case NCOMPONENT:
					{
						const size_t cChannel = pSC->WMISCP.cChannel; // check xxx => Y_ONLY transcoding!
						size_t iChannel;

						for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cStride){
							iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
							for(iChannel = 0; iChannel < cChannel; iChannel ++)
								pSC->p1MBbuffer[iChannel][iPos] = JxrEncoderSampleConversionFromHalf(pSrc[iChannel]) << cShift;
						}
					}
					break;

                default:
                    assert(0);
                    break;
            }
        }
        else if(bdExt == BD_32){
            const U32 * pSrc = (U32 *)pSrc0 + pSC->WMII.cLeadingPadding;
            const size_t cStride = cPixelStride / sizeof(U32);
            const PixelI iOffset = ((1 << 31) >> nLen) << cShift;

            switch(cfExt){
                case CF_RGB:
                    for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cStride){
                        PixelI r = (pSrc[0] >> nLen) << cShift, g = (pSrc[1] >> nLen) << cShift, b = (pSrc[2] >> nLen) << cShift;

                        JxrEncoderColorTransformApplyRgb(&r, &g, &b); // color conversion

                        iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
                        pU[iPos] = -r, pV[iPos] = b, pY[iPos] = g - iOffset;
                    }
                    break;

                case Y_ONLY:
                case YUV_444:
                case NCOMPONENT:
                {
                    const size_t cChannel = pSC->WMISCP.cChannel;
                    size_t iChannel;

                    for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cStride){
                        iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
                        for(iChannel = 0; iChannel < cChannel; iChannel ++)
                            pSC->p1MBbuffer[iChannel][iPos] = (pSrc[iChannel] >> nLen) << cShift;
                    }
                    break;
                }

                default:
                    assert(0);
                    break;
            }
        }
        else if(bdExt == BD_32S){
            const I32 * pSrc = (I32 *)pSrc0 + pSC->WMII.cLeadingPadding;
            const size_t cStride = cPixelStride / sizeof(I32);

            switch(cfExt){
                case CF_RGB:
                    for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cStride){
                        PixelI r = (pSrc[0] >> nLen)<< cShift, g = (pSrc[1] >> nLen)<< cShift, b = (pSrc[2] >> nLen)<< cShift;

                        JxrEncoderColorTransformApplyRgb(&r, &g, &b); // color conversion

                        iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
                        pU[iPos] = -r, pV[iPos] = b, pY[iPos] = g;
                    }
                    break;

                case Y_ONLY:
                case YUV_444:
                case NCOMPONENT:
					{
						const size_t cChannel = pSC->WMISCP.cChannel; // check xxx => Y_ONLY transcoding!
						size_t iChannel;

						for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cStride){
							iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
							for(iChannel = 0; iChannel < cChannel; iChannel ++)
								pSC->p1MBbuffer[iChannel][iPos] = (pSrc[iChannel] >> nLen) << cShift;
						}
					}
					break;

                default:
                    assert(0);
                    break;
            }
        }
        else if(bdExt == BD_32F){
            const float * pSrc = (float *)pSrc0 + pSC->WMII.cLeadingPadding;
            const size_t cStride = cPixelStride / sizeof(float);

            switch(cfExt){
                case CF_RGB:
                    for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cStride){
                        PixelI r = JxrEncoderSampleConversionFromSingle(pSrc[0], nExpBias, nLen) << cShift;
                        PixelI g = JxrEncoderSampleConversionFromSingle(pSrc[1], nExpBias, nLen) << cShift;
                        PixelI b = JxrEncoderSampleConversionFromSingle(pSrc[2], nExpBias, nLen) << cShift;

                        JxrEncoderColorTransformApplyRgb(&r, &g, &b); // color conversion

                        iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
                        pU[iPos] = -r, pV[iPos] = b, pY[iPos] = g;
                    }
                    break;

                case Y_ONLY:
                case YUV_444:
                case NCOMPONENT:
					{
						const size_t cChannel = pSC->WMISCP.cChannel;
						size_t iChannel;

						for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cStride){
							iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
							for(iChannel = 0; iChannel < cChannel; iChannel ++)
								pSC->p1MBbuffer[iChannel][iPos] = JxrEncoderSampleConversionFromSingle(pSrc[iChannel], nExpBias, nLen) << cShift;
						}
					}
					break;
                default:
                    assert(0);
                    break;
            }
        }
        else if(bdExt == BD_5){ // RGB 555, work for both big endian and small endian!
            const U8 * pSrc = pSrc0;
            const PixelI iOffset = (16 << cShift);

            assert(cfExt == CF_RGB);

            for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cPixelStride){
                PixelI r = (PixelI)pSrc[0], g = (PixelI)pSrc[1], b = ((g >> 2) & 0x1F) << cShift;

                g = ((r >> 5) + ((g & 3) << 3)) << cShift, r = (r & 0x1F) << cShift;

                JxrEncoderColorTransformApplyRgb(&r, &g, &b); // color conversion

                iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
                pU[iPos] = -r, pV[iPos] = b, pY[iPos] = g - iOffset;
            }
        }
        else if(bdExt == BD_565){ // RGB 555, work for both big endian and small endian!
            const U8 * pSrc = pSrc0;
            const PixelI iOffset = (32 << cShift);

            assert(cfExt == CF_RGB);

            for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cPixelStride){
                PixelI r = (PixelI)pSrc[0], g = (PixelI)pSrc[1], b = (g >> 3) << (cShift + 1);

                g = ((r >> 5) + ((g & 7) << 3)) << cShift, r = (r & 0x1F) << (cShift + 1);

                JxrEncoderColorTransformApplyRgb(&r, &g, &b); // color conversion

                iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
                pU[iPos] = -r, pV[iPos] = b, pY[iPos] = g - iOffset;
            }
        }
        else if(bdExt == BD_10){ //RGB 101010, work for both big endian and small endian!
            const U8 * pSrc = pSrc0;
            const PixelI iOffset = (512 << cShift);

            assert(cfExt == CF_RGB);

            for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cPixelStride){
                PixelI r = (PixelI)pSrc[0], g = (PixelI)pSrc[1], b = (PixelI)pSrc[2];

                r = (r + ((g & 3) << 8)) << cShift, g = ((g >> 2) + ((b & 0xF) << 6)) << cShift;
                b = ((b >> 4) + (((PixelI)pSrc[3] & 0x3F) << 4)) << cShift;

                JxrEncoderColorTransformApplyRgb(&r, &g, &b); // color conversion

                iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
                pU[iPos] = -r, pV[iPos] = b, pY[iPos] = g - iOffset;
            }
        }
        else if(bdExt == BD_1){
            assert(cfExt == Y_ONLY);
            for(iColumn = 0; iColumn < cColumn; iColumn ++) {
                pY[((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf]] = ((pSC->WMISCP.bBlackWhite + (pSrc0[iColumn >> 3] >> (7 - (iColumn & 7)))) & 1) << cShift;
            }
        }

        if(iRow + iRowStride < cRow) // centralized vertical padding!
            pSrc0 += pSC->WMIBI.cbStride;
    }

    JxrEncoderInputPaddingApply(pSC); // centralized horizontal padding

    // centralized down-sampling
    if(pSC->m_bUVResolutionChange)
        JxrEncoderChromaDownsamplerApply(pSC);

    // centralized alpha channel handdling
    if (pSC->WMISCP.uAlphaMode == 3)
        if(JxrEncoderAlphaPlaneInputRead(pSC) != ICERR_OK)
            return ICERR_ERROR;

    return ICERR_OK;
}
