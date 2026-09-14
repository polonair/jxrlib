#include "JxrEncoderCoefficientPredictor.h"
#include "encode.h"

static Void JxrEncoderCoefficientPredictorApplyFullResolutionAc(
    PixelI* coefficients, Int acMode)
{
    Int macroblockRow;
    Int blockRow;

    if (acMode == 1) {
        for (macroblockRow = 0; macroblockRow <= 192; macroblockRow += 64) {
            for (blockRow = 48; blockRow > 0; blockRow -= 16) {
                coefficients[macroblockRow + blockRow + 10] -=
                    coefficients[macroblockRow + blockRow - 6];
                coefficients[macroblockRow + blockRow + 2] -=
                    coefficients[macroblockRow + blockRow - 14];
                coefficients[macroblockRow + blockRow + 9] -=
                    coefficients[macroblockRow + blockRow - 7];
            }
        }
    }
    else if (acMode == 0) {
        for (macroblockRow = 0; macroblockRow < 64; macroblockRow += 16) {
            for (blockRow = 192; blockRow > 0; blockRow -= 64) {
                coefficients[macroblockRow + blockRow + 5] -=
                    coefficients[macroblockRow + blockRow - 59];
                coefficients[macroblockRow + blockRow + 1] -=
                    coefficients[macroblockRow + blockRow - 63];
                coefficients[macroblockRow + blockRow + 6] -=
                    coefficients[macroblockRow + blockRow - 58];
            }
        }
    }
}

Void JxrEncoderCoefficientPredictorApplyFullResolution(PixelI* dcCoefficients,
    PixelI* macroblockCoefficients, Int dcMode, Int adMode, Int acMode,
    const JxrEncoderCoefficientPredictionReferences* references)
{
    if (dcMode == 1)
        dcCoefficients[0] -= references->topDc;
    else if (dcMode == 0)
        dcCoefficients[0] -= references->leftDc;
    else if (dcMode == 2)
        dcCoefficients[0] -= (references->leftDc + references->topDc) >> 1;

    if (adMode == 4) {
        dcCoefficients[4] -= references->topAd[3];
        dcCoefficients[8] -= references->topAd[4];
        dcCoefficients[12] -= references->topAd[5];
    }
    else if (adMode == 0) {
        dcCoefficients[1] -= references->leftAd[0];
        dcCoefficients[2] -= references->leftAd[1];
        dcCoefficients[3] -= references->leftAd[2];
    }

    JxrEncoderCoefficientPredictorApplyFullResolutionAc(macroblockCoefficients,
        acMode);
}

Void JxrEncoderCoefficientPredictorApplyChroma420(PixelI* dcCoefficients,
    PixelI* macroblockCoefficients, Int dcMode, Int adMode, Int acMode,
    const JxrEncoderCoefficientPredictionReferences* references)
{
    Int blockOffset;

    if (dcMode == 1)
        dcCoefficients[0] -= references->topDc;
    else if (dcMode == 0)
        dcCoefficients[0] -= references->leftDc;
    else if (dcMode == 2)
        dcCoefficients[0] -= (references->leftDc + references->topDc + 1) >> 1;

    if (adMode == 4)
        dcCoefficients[2] -= references->topAd[1];
    else if (adMode == 0)
        dcCoefficients[1] -= references->leftAd[0];

    if (acMode == 1) {
        for (blockOffset = 16; blockOffset <= 48; blockOffset += 32) {
            macroblockCoefficients[blockOffset + 10] -=
                macroblockCoefficients[blockOffset - 6];
            macroblockCoefficients[blockOffset + 2] -=
                macroblockCoefficients[blockOffset - 14];
            macroblockCoefficients[blockOffset + 9] -=
                macroblockCoefficients[blockOffset - 7];
        }
    }
    else if (acMode == 0) {
        for (blockOffset = 32; blockOffset <= 48; blockOffset += 16) {
            macroblockCoefficients[blockOffset + 5] -=
                macroblockCoefficients[blockOffset - 27];
            macroblockCoefficients[blockOffset + 1] -=
                macroblockCoefficients[blockOffset - 31];
            macroblockCoefficients[blockOffset + 6] -=
                macroblockCoefficients[blockOffset - 26];
        }
    }
}

Void JxrEncoderCoefficientPredictorApplyChroma422(PixelI* dcCoefficients,
    PixelI* macroblockCoefficients, Int dcMode, Int adMode, Int acMode,
    const JxrEncoderCoefficientPredictionReferences* references)
{
    Int blockIndex;
    Int blockOffset;

    if (dcMode == 1)
        dcCoefficients[0] -= references->topDc;
    else if (dcMode == 0)
        dcCoefficients[0] -= references->leftDc;
    else if (dcMode == 2)
        dcCoefficients[0] -= (references->leftDc + references->topDc + 1) >> 1;

    if (adMode == 4) {
        dcCoefficients[4] -= references->topAd[4];
        dcCoefficients[6] -= dcCoefficients[2];
        dcCoefficients[2] -= references->topAd[3];
    }
    else if (adMode == 0) {
        dcCoefficients[4] -= references->leftAd[4];
        dcCoefficients[1] -= references->leftAd[0];
        dcCoefficients[5] -= references->leftAd[2];
    }
    else if (dcMode == 1)
        dcCoefficients[6] -= dcCoefficients[2];

    if (acMode == 1) {
        for (blockIndex = 2; blockIndex < 8; blockIndex++) {
            blockOffset = blkOffsetUV_422[blockIndex];
            macroblockCoefficients[blockOffset + 10] -=
                macroblockCoefficients[blockOffset - 6];
            macroblockCoefficients[blockOffset + 2] -=
                macroblockCoefficients[blockOffset - 14];
            macroblockCoefficients[blockOffset + 9] -=
                macroblockCoefficients[blockOffset - 7];
        }
    }
    else if (acMode == 0) {
        for (blockIndex = 1; blockIndex < 8; blockIndex += 2) {
            blockOffset = blkOffsetUV_422[blockIndex];
            macroblockCoefficients[blockOffset + 5] -=
                macroblockCoefficients[blockOffset - 59];
            macroblockCoefficients[blockOffset + 1] -=
                macroblockCoefficients[blockOffset - 63];
            macroblockCoefficients[blockOffset + 6] -=
                macroblockCoefficients[blockOffset - 58];
        }
    }
}

static Void JxrEncoderCoefficientPredictorResolveReferences(
    CWMImageStrCodec* codec, Int channel, size_t column, Int dcMode,
    Int adMode, JxrEncoderCoefficientPredictionReferences* references)
{
    references->leftDc = 0;
    references->topDc = 0;
    references->leftAd = NULL;
    references->topAd = NULL;

    if (dcMode == 0 || dcMode == 2 || adMode == 0) {
        references->leftDc = codec->PredInfo[channel][column - 1].iDC;
        references->leftAd = codec->PredInfo[channel][column - 1].piAD;
    }
    if (dcMode == 1 || dcMode == 2 || adMode == 4) {
        references->topDc = codec->PredInfoPrevRow[channel][column].iDC;
        references->topAd = codec->PredInfoPrevRow[channel][column].piAD;
    }
}

Void JxrEncoderCoefficientPredictorApply(CWMImageStrCodec* codec)
{
    COLORFORMAT colorFormat = codec->m_param.cfColorFormat;
    Int primaryChannelCount = (colorFormat == YUV_420 || colorFormat == YUV_422) ?
        1 : (Int)codec->m_param.cNumChannels;
    size_t macroblockColumn = codec->cColumn - 1;
    CWMIMBInfo* macroblockInfo = &codec->MBInfo;
    Int predictionMode = getDCACPredMode(codec, macroblockColumn);
    Int dcMode = predictionMode & 0x3;
    Int adMode = predictionMode & 0xC;
    Int acMode = getACPredMode(macroblockInfo, colorFormat);
    JxrEncoderCoefficientPredictionReferences references;
    Int channel;

    macroblockInfo->iOrientation = 2 - acMode;
    updatePredInfo(codec, macroblockInfo, macroblockColumn, colorFormat);

    for (channel = 0; channel < primaryChannelCount; channel++) {
        JxrEncoderCoefficientPredictorResolveReferences(codec, channel,
            macroblockColumn, dcMode, adMode, &references);
        JxrEncoderCoefficientPredictorApplyFullResolution(
            macroblockInfo->iBlockDC[channel], codec->pPlane[channel], dcMode,
            adMode, acMode, &references);
    }

    if (colorFormat == YUV_420) {
        for (channel = 1; channel < 3; channel++) {
            JxrEncoderCoefficientPredictorResolveReferences(codec, channel,
                macroblockColumn, dcMode, adMode, &references);
            JxrEncoderCoefficientPredictorApplyChroma420(
                macroblockInfo->iBlockDC[channel], codec->pPlane[channel], dcMode,
                adMode, acMode, &references);
        }
    }
    else if (colorFormat == YUV_422) {
        for (channel = 1; channel < 3; channel++) {
            JxrEncoderCoefficientPredictorResolveReferences(codec, channel,
                macroblockColumn, dcMode, adMode, &references);
            JxrEncoderCoefficientPredictorApplyChroma422(
                macroblockInfo->iBlockDC[channel], codec->pPlane[channel], dcMode,
                adMode, acMode, &references);
        }
    }
}
