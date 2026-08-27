#include "JxrDecoderNChannelOutputWriter.h"
#include "decode.h"
#include "JxrSampleClipping.h"
#include "JxrFloatSampleConversion.h"
Void JxrDecoderNChannelOutputWriterWrite(CWMImageStrCodec * pSC, size_t iFirstRow, size_t iFirstColumn, size_t cWidth, size_t cHeight, size_t iShift, PixelI iBias)
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
