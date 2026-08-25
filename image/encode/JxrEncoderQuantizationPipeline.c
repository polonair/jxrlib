#include "JxrEncoderQuantizationPipeline.h"

#include "encode.h"

static I32 JxrEncoderQuantizationPipelineQuantizeMulless(PixelI value,
    PixelI offset, I32 exponent)
{
    const I32 signMask = value >> 31;

    assert(sizeof(PixelI) == sizeof(U32));
    return ((((value ^ signMask) - signMask + offset) >> exponent) ^ signMask) - signMask;
}

static I32 JxrEncoderQuantizationPipelineMultiplyHigh(U32 value, U32 multiplier,
    U32 exponent)
{
    return (I32)((U32)((U64)value * multiplier >> 32) >> exponent);
}

static I32 JxrEncoderQuantizationPipelineQuantizeValue(PixelI value,
    PixelI offset, I32 multiplier, I32 exponent)
{
    const I32 signMask = value >> 31;

    assert(sizeof(PixelI) == sizeof(U32));
    return (JxrEncoderQuantizationPipelineMultiplyHigh(
        (value ^ signMask) - signMask + offset, multiplier, exponent) ^ signMask) - signMask;
}

Void JxrEncoderQuantizationPipelineQuantize(CWMImageStrCodec* codec)
{
    CWMITile* tile = codec->pTile + codec->cTileColumn;
    CWMIMBInfo* macroblockInfo = &codec->MBInfo;
    const COLORFORMAT colorFormat = codec->m_param.cfColorFormat;
    int channelIndex;
    int coefficientIndex;
    int blockIndex;

    if (codec->m_param.bTranscode == FALSE) {
        for (channelIndex = 0; channelIndex < (int)codec->m_param.cNumChannels; ++channelIndex) {
            const Bool isChroma = channelIndex > 0 && (colorFormat == YUV_444 ||
                colorFormat == YUV_422 || colorFormat == YUV_420);
            const int blockCount = isChroma ? (colorFormat == YUV_422 ? 8 :
                (colorFormat == YUV_420 ? 4 : 16)) : 16;
            const int* blockOffsets = blockCount == 4 ? blkOffsetUV :
                (blockCount == 8 ? blkOffsetUV_422 : blkOffset);
            CWMIQuantizer* dcQuantizer = tile->pQuantizerDC[channelIndex];
            CWMIQuantizer* lpQuantizer = tile->pQuantizerLP[channelIndex] +
                macroblockInfo->iQIndexLP;
            CWMIQuantizer* hpQuantizer = tile->pQuantizerHP[channelIndex] +
                macroblockInfo->iQIndexHP;

            for (blockIndex = 0; blockIndex < blockCount; ++blockIndex) {
                PixelI* coefficients = codec->pPlane[channelIndex] + blockOffsets[blockIndex];

                if (blockIndex == 0) {
                    coefficients[0] = dcQuantizer->iMan == 0 ?
                        JxrEncoderQuantizationPipelineQuantizeMulless(coefficients[0],
                            dcQuantizer->iOffset, dcQuantizer->iExp) :
                        JxrEncoderQuantizationPipelineQuantizeValue(coefficients[0],
                            dcQuantizer->iOffset, dcQuantizer->iMan, dcQuantizer->iExp);
                }
                else if (codec->WMISCP.sbSubband != SB_DC_ONLY) {
                    coefficients[0] = lpQuantizer->iMan == 0 ?
                        JxrEncoderQuantizationPipelineQuantizeMulless(coefficients[0],
                            lpQuantizer->iOffset, lpQuantizer->iExp) :
                        JxrEncoderQuantizationPipelineQuantizeValue(coefficients[0],
                            lpQuantizer->iOffset, lpQuantizer->iMan, lpQuantizer->iExp);
                }

                if (codec->WMISCP.sbSubband != SB_DC_ONLY &&
                    codec->WMISCP.sbSubband != SB_NO_HIGHPASS) {
                    for (coefficientIndex = 1; coefficientIndex < 16; ++coefficientIndex) {
                        coefficients[coefficientIndex] = hpQuantizer->iMan == 0 ?
                            JxrEncoderQuantizationPipelineQuantizeMulless(
                                coefficients[coefficientIndex], hpQuantizer->iOffset,
                                hpQuantizer->iExp) :
                            JxrEncoderQuantizationPipelineQuantizeValue(
                                coefficients[coefficientIndex], hpQuantizer->iOffset,
                                hpQuantizer->iMan, hpQuantizer->iExp);
                    }
                }
            }
        }
    }

    for (channelIndex = 0; channelIndex < (int)codec->m_param.cNumChannels; ++channelIndex) {
        I32* dcCoefficients = codec->MBInfo.iBlockDC[channelIndex];
        PixelI* coefficients = codec->pPlane[channelIndex];
        const int dcBlockCount = channelIndex > 0 && colorFormat == YUV_422 ? 8 :
            (channelIndex > 0 && colorFormat == YUV_420 ? 4 : 16);
        const int* dcOffsets = dcBlockCount == 4 ? blkOffsetUV :
            (dcBlockCount == 8 ? blkOffsetUV_422 : dctIndex[2]);

        for (blockIndex = 0; blockIndex < dcBlockCount; ++blockIndex)
            dcCoefficients[blockIndex] = coefficients[dcOffsets[blockIndex]];
    }
}
