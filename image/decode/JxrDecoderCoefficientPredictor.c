#include "JxrDecoderCoefficientPredictor.h"

Void JxrDecoderCoefficientPredictorApplyDcAc(CWMImageStrCodec* codec)
{
    const COLORFORMAT colorFormat = codec->m_param.cfColorFormat;
    const Int primaryChannelCount = (colorFormat == YUV_420 || colorFormat == YUV_422) ?
        1 : (Int)codec->m_param.cNumChannels;
    CWMIMBInfo* macroblockInfo = &codec->MBInfo;
    size_t macroblockColumn = codec->cColumn;
    Int predictionMode = getDCACPredMode(codec, macroblockColumn);
    Int dcPredictionMode = predictionMode & 0x3;
    Int adPredictionMode = predictionMode & 0xC;
    PixelI* coefficients;
    PixelI* referenceCoefficients;
    Int channel;

    for (channel = 0; channel < primaryChannelCount; channel++) {
        coefficients = macroblockInfo->iBlockDC[channel];
        if (dcPredictionMode == 1)
            coefficients[0] += codec->PredInfoPrevRow[channel][macroblockColumn].iDC;
        else if (dcPredictionMode == 0)
            coefficients[0] += codec->PredInfo[channel][macroblockColumn - 1].iDC;
        else if (dcPredictionMode == 2)
            coefficients[0] += (codec->PredInfo[channel][macroblockColumn - 1].iDC +
                codec->PredInfoPrevRow[channel][macroblockColumn].iDC) >> 1;

        if (adPredictionMode == 4) {
            referenceCoefficients = codec->PredInfoPrevRow[channel][macroblockColumn].piAD;
            coefficients[4] += referenceCoefficients[3];
            coefficients[8] += referenceCoefficients[4];
            coefficients[12] += referenceCoefficients[5];
        }
        else if (adPredictionMode == 0) {
            referenceCoefficients = codec->PredInfo[channel][macroblockColumn - 1].piAD;
            coefficients[1] += referenceCoefficients[0];
            coefficients[2] += referenceCoefficients[1];
            coefficients[3] += referenceCoefficients[2];
        }
    }

    if (colorFormat == YUV_420) {
        for (channel = 1; channel < 3; channel++) {
            coefficients = macroblockInfo->iBlockDC[channel];
            if (dcPredictionMode == 1)
                coefficients[0] += codec->PredInfoPrevRow[channel][macroblockColumn].iDC;
            else if (dcPredictionMode == 0)
                coefficients[0] += codec->PredInfo[channel][macroblockColumn - 1].iDC;
            else if (dcPredictionMode == 2)
                coefficients[0] += (codec->PredInfo[channel][macroblockColumn - 1].iDC +
                    codec->PredInfoPrevRow[channel][macroblockColumn].iDC + 1) >> 1;

            if (adPredictionMode == 4)
                coefficients[2] += codec->PredInfoPrevRow[channel][macroblockColumn].piAD[1];
            else if (adPredictionMode == 0)
                coefficients[1] += codec->PredInfo[channel][macroblockColumn - 1].piAD[0];
        }
    }
    else if (colorFormat == YUV_422) {
        for (channel = 1; channel < 3; channel++) {
            coefficients = macroblockInfo->iBlockDC[channel];
            if (dcPredictionMode == 1)
                coefficients[0] += codec->PredInfoPrevRow[channel][macroblockColumn].iDC;
            else if (dcPredictionMode == 0)
                coefficients[0] += codec->PredInfo[channel][macroblockColumn - 1].iDC;
            else if (dcPredictionMode == 2)
                coefficients[0] += (codec->PredInfo[channel][macroblockColumn - 1].iDC +
                    codec->PredInfoPrevRow[channel][macroblockColumn].iDC + 1) >> 1;

            if (adPredictionMode == 4) {
                coefficients[4] += codec->PredInfoPrevRow[channel][macroblockColumn].piAD[4];
                coefficients[2] += codec->PredInfoPrevRow[channel][macroblockColumn].piAD[3];
                coefficients[6] += coefficients[2];
            }
            else if (adPredictionMode == 0) {
                referenceCoefficients = codec->PredInfo[channel][macroblockColumn - 1].piAD;
                coefficients[4] += referenceCoefficients[4];
                coefficients[1] += referenceCoefficients[0];
                coefficients[5] += referenceCoefficients[2];
            }
            else if (dcPredictionMode == 1)
                coefficients[6] += coefficients[2];
        }
    }
    macroblockInfo->iOrientation = 2 - getACPredMode(macroblockInfo, colorFormat);
}

Void JxrDecoderCoefficientPredictorApplyAc(CWMImageStrCodec* codec)
{
    static const U8 fullResolutionTopBlockIndexes[] =
        { 1, 2, 3, 5, 6, 7, 9, 10, 11, 13, 14, 15 };
    const COLORFORMAT colorFormat = codec->m_param.cfColorFormat;
    const Int primaryChannelCount = (colorFormat == YUV_420 || colorFormat == YUV_422) ?
        1 : (Int)codec->m_param.cNumChannels;
    Int acPredictionMode = 2 - codec->MBInfo.iOrientation;
    PixelI* coefficients;
    PixelI* referenceCoefficients;
    Int channel;
    Int block;

    for (channel = 0; channel < primaryChannelCount; channel++) {
        PixelI* macroblockBuffer = codec->p1MBbuffer[channel];

        if (acPredictionMode == 1) {
            for (block = 0; block < sizeof(fullResolutionTopBlockIndexes) /
                sizeof(fullResolutionTopBlockIndexes[0]); block++) {
                coefficients = macroblockBuffer + 16 * fullResolutionTopBlockIndexes[block];
                referenceCoefficients = coefficients - 16;
                coefficients[2] += referenceCoefficients[2];
                coefficients[10] += referenceCoefficients[10];
                coefficients[9] += referenceCoefficients[9];
            }
        }
        else if (acPredictionMode == 0) {
            for (block = 64; block < 256; block += 16) {
                coefficients = macroblockBuffer + block;
                referenceCoefficients = coefficients - 64;
                coefficients[1] += referenceCoefficients[1];
                coefficients[5] += referenceCoefficients[5];
                coefficients[6] += referenceCoefficients[6];
            }
        }
    }

    if (colorFormat == YUV_420) {
        for (channel = 1; channel < 3; channel++) {
            PixelI* macroblockBuffer = codec->p1MBbuffer[channel];
            if (acPredictionMode == 1) {
                for (block = 1; block <= 3; block += 2) {
                    coefficients = macroblockBuffer + 16 * block;
                    referenceCoefficients = coefficients - 16;
                    coefficients[2] += referenceCoefficients[2];
                    coefficients[10] += referenceCoefficients[10];
                    coefficients[9] += referenceCoefficients[9];
                }
            }
            else if (acPredictionMode == 0) {
                for (block = 2; block <= 3; block++) {
                    coefficients = macroblockBuffer + 16 * block;
                    referenceCoefficients = coefficients - 32;
                    coefficients[1] += referenceCoefficients[1];
                    coefficients[5] += referenceCoefficients[5];
                    coefficients[6] += referenceCoefficients[6];
                }
            }
        }
    }
    else if (colorFormat == YUV_422) {
        for (channel = 1; channel < 3; channel++) {
            PixelI* macroblockBuffer = codec->p1MBbuffer[channel];
            if (acPredictionMode == 1) {
                for (block = 2; block < 8; block++) {
                    coefficients = macroblockBuffer + blkOffsetUV_422[block];
                    referenceCoefficients = coefficients - 16;
                    coefficients[10] += referenceCoefficients[10];
                    coefficients[2] += referenceCoefficients[2];
                    coefficients[9] += referenceCoefficients[9];
                }
            }
            else if (acPredictionMode == 0) {
                for (block = 1; block < 8; block += 2) {
                    coefficients = macroblockBuffer + blkOffsetUV_422[block];
                    referenceCoefficients = coefficients - 64;
                    coefficients[1] += referenceCoefficients[1];
                    coefficients[5] += referenceCoefficients[5];
                    coefficients[6] += referenceCoefficients[6];
                }
            }
        }
    }
}
