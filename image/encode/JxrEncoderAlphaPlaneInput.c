#include "JxrEncoderAlphaPlaneInput.h"
#include "JxrEncoderSampleConversion.h"

Void JxrEncoderAlphaPlaneInputPlanInitialize(JxrEncoderAlphaPlaneInputPlan* plan,
    Bool isSecondaryCodec, Bool hasSecondaryCodec, BITDEPTH_BITS sourceBitDepth)
{
    plan->readsAlphaPlane = !isSecondaryCodec && hasSecondaryCodec;
    plan->supportsSourceBitDepth = sourceBitDepth == BD_8 || sourceBitDepth == BD_16 ||
        sourceBitDepth == BD_16S || sourceBitDepth == BD_16F ||
        sourceBitDepth == BD_32S || sourceBitDepth == BD_32F;
}

Int JxrEncoderAlphaPlaneInputRead(CWMImageStrCodec* codec)
{
    JxrEncoderAlphaPlaneInputPlan plan;

    JxrEncoderAlphaPlaneInputPlanInitialize(&plan, codec->m_bSecondary,
        codec->m_pNextSC != NULL, codec->WMII.bdBitDepth);
    if (!plan.readsAlphaPlane)
        return ICERR_OK;
    if (!plan.supportsSourceBitDepth)
        return ICERR_ERROR;

    {
        const size_t sampleShift = codec->m_pNextSC->m_param.bScaledArith ?
            SHIFTZERO + QPFRACBITS : 0;
        const BITDEPTH_BITS sourceBitDepth = codec->WMII.bdBitDepth;
        const size_t alphaComponentIndex = codec->WMII.cLeadingPadding +
            (codec->WMII.cfColorFormat == CMYK ? 4 : 3);
        const size_t sourceRowCount = codec->WMIBI.cLine;
        const size_t sourceColumnCount = codec->WMII.cWidth;
        const U8* sourceRow = (U8*)codec->WMIBI.pv;
        PixelI* alphaSamples = codec->m_pNextSC->p1MBbuffer[0];
        size_t row;
        size_t column;

        for (row = 0; row < 16; ++row) {
            if (sourceBitDepth == BD_8) {
                const size_t stride = codec->WMII.cBitsPerUnit >> 3;
                const U8* source = sourceRow;
                for (column = 0; column < sourceColumnCount; ++column, source += stride)
                    alphaSamples[((column >> 4) << 8) + idxCC[row][column & 0xf]] =
                        ((PixelI)source[alphaComponentIndex] - (1 << 7)) << sampleShift;
            }
            else if (sourceBitDepth == BD_16) {
                const size_t stride = (codec->WMII.cBitsPerUnit >> 3) / sizeof(U16);
                const U8 shift = codec->m_pNextSC->WMISCP.nLenMantissaOrShift;
                const U16* source = (U16*)sourceRow;
                for (column = 0; column < sourceColumnCount; ++column, source += stride)
                    alphaSamples[((column >> 4) << 8) + idxCC[row][column & 0xf]] =
                        (((PixelI)source[alphaComponentIndex] - (1 << 15)) >> shift) << sampleShift;
            }
            else if (sourceBitDepth == BD_16S) {
                const size_t stride = (codec->WMII.cBitsPerUnit >> 3) / sizeof(I16);
                const U8 shift = codec->m_pNextSC->WMISCP.nLenMantissaOrShift;
                const I16* source = (I16*)sourceRow;
                for (column = 0; column < sourceColumnCount; ++column, source += stride)
                    alphaSamples[((column >> 4) << 8) + idxCC[row][column & 0xf]] =
                        ((PixelI)source[alphaComponentIndex] >> shift) << sampleShift;
            }
            else if (sourceBitDepth == BD_16F) {
                const size_t stride = (codec->WMII.cBitsPerUnit >> 3) / sizeof(U16);
                const I16* source = (I16*)sourceRow;
                for (column = 0; column < sourceColumnCount; ++column, source += stride)
                    alphaSamples[((column >> 4) << 8) + idxCC[row][column & 0xf]] =
                        JxrEncoderSampleConversionFromHalf(source[alphaComponentIndex]) << sampleShift;
            }
            else if (sourceBitDepth == BD_32S) {
                const size_t stride = (codec->WMII.cBitsPerUnit >> 3) / sizeof(I32);
                const U8 shift = codec->m_pNextSC->WMISCP.nLenMantissaOrShift;
                const I32* source = (I32*)sourceRow;
                for (column = 0; column < sourceColumnCount; ++column, source += stride)
                    alphaSamples[((column >> 4) << 8) + idxCC[row][column & 0xf]] =
                        ((PixelI)source[alphaComponentIndex] >> shift) << sampleShift;
            }
            else {
                const size_t stride = (codec->WMII.cBitsPerUnit >> 3) / sizeof(float);
                const U8 mantissaLength = codec->m_pNextSC->WMISCP.nLenMantissaOrShift;
                const I8 exponentBias = codec->m_pNextSC->WMISCP.nExpBias;
                const float* source = (float*)sourceRow;
                for (column = 0; column < sourceColumnCount; ++column, source += stride)
                    alphaSamples[((column >> 4) << 8) + idxCC[row][column & 0xf]] =
                        JxrEncoderSampleConversionFromSingle(source[alphaComponentIndex],
                            exponentBias, mantissaLength) << sampleShift;
            }

            if (row + 1 < sourceRowCount)
                sourceRow += codec->WMIBI.cbStride;

            for (column = sourceColumnCount; column < codec->cmbWidth * 16; ++column)
                alphaSamples[((column >> 4) << 8) + idxCC[row][column & 0xf]] =
                    alphaSamples[(((sourceColumnCount - 1) >> 4) << 8) +
                        idxCC[row][(sourceColumnCount - 1) & 0xf]];
        }
    }

    return ICERR_OK;
}
