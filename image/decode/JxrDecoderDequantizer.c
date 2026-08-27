#include "JxrDecoderDequantizer.h"
#include "JxrPredictionMath.h"

Void JxrDecoderDequantizerWrite4x4(PixelI* destination,
    const Int* coefficients, const Int* coefficientIndexes, Int quantizationParameter)
{
    Int coefficientIndex;

    for (coefficientIndex = 1; coefficientIndex < 16; coefficientIndex++)
        destination[coefficientIndexes[coefficientIndex]] =
            JxrPredictionMathDequantize(coefficients[coefficientIndex], quantizationParameter);
}

Void JxrDecoderDequantizerWrite2x2(PixelI* destination,
    const Int* coefficients, Int quantizationParameter)
{
    destination[32] = JxrPredictionMathDequantize(coefficients[1], quantizationParameter);
    destination[16] = JxrPredictionMathDequantize(coefficients[2], quantizationParameter);
    destination[48] = JxrPredictionMathDequantize(coefficients[3], quantizationParameter);
}

Void JxrDecoderDequantizerWrite4x2(PixelI* destination,
    const Int* coefficients, Int quantizationParameter)
{
    destination[64] = JxrPredictionMathDequantize(coefficients[1], quantizationParameter);
    destination[16] = JxrPredictionMathDequantize(coefficients[2], quantizationParameter);
    destination[80] = JxrPredictionMathDequantize(coefficients[3], quantizationParameter);
    destination[32] = JxrPredictionMathDequantize(coefficients[4], quantizationParameter);
    destination[96] = JxrPredictionMathDequantize(coefficients[5], quantizationParameter);
    destination[48] = JxrPredictionMathDequantize(coefficients[6], quantizationParameter);
    destination[112] = JxrPredictionMathDequantize(coefficients[7], quantizationParameter);
}

Int JxrDecoderDequantizerDequantizeMacroblock(CWMImageStrCodec* codec)
{
    const COLORFORMAT colorFormat = codec->m_param.cfColorFormat;
    CWMIMBInfo* macroblockInfo = &codec->MBInfo;
    CWMITile* currentTile = &codec->pTile[codec->cTileColumn];
    size_t channel;

    for (channel = 0; channel < codec->m_param.cNumChannels; channel++) {
        PixelI* macroblockBuffer = codec->p1MBbuffer[channel];
        const Int* coefficients = macroblockInfo->iBlockDC[channel];
        Int lowpassQuantizationParameter;

        macroblockBuffer[0] = JxrPredictionMathDequantize(coefficients[0],
            currentTile->pQuantizerDC[channel]->iQP);
        if (codec->WMISCP.sbSubband == SB_DC_ONLY) continue;
        lowpassQuantizationParameter = currentTile->pQuantizerLP[channel]
            [macroblockInfo->iQIndexLP].iQP;
        if (channel == 0 || (colorFormat != YUV_422 && colorFormat != YUV_420))
            JxrDecoderDequantizerWrite4x4(macroblockBuffer, coefficients, dctIndex[2],
                lowpassQuantizationParameter);
        else if (colorFormat == YUV_422)
            JxrDecoderDequantizerWrite4x2(macroblockBuffer, coefficients,
                lowpassQuantizationParameter);
        else
            JxrDecoderDequantizerWrite2x2(macroblockBuffer, coefficients,
                lowpassQuantizationParameter);
    }
    return ICERR_OK;
}
