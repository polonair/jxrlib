#include "JxrEncoderCbpPredictor.h"
#include "encode.h"

/* CBP prediction for 16 x 16 MB */
/* block index */
/*  0  1  4  5 */
/*  2  3  6  7 */
/*  8  9 12 13 */
/* 10 11 14 15 */

static int NumOnes(int i)
{
    int retval = 0;
    static const int g_Count[] = { 0,1,1,2, 1,2,2,3, 1,2,2,3, 2,3,3,4 };
    i = i & 0xffff;
    while (i) {
        retval += g_Count[i & 0xf];
        i >>= 4;
    }
    return retval;
}

Int JxrEncoderCbpPredictorClampModelCount(Int value)
{
    if (value < -16)
        return -16;
    if (value > 15)
        return 15;
    return value;
}

Void JxrEncoderCbpPredictorUpdateModel(CCBPModel* model, size_t modelIndex,
    Int originalCoefficientCount)
{
    model->m_iCount0[modelIndex] = JxrEncoderCbpPredictorClampModelCount(
        model->m_iCount0[modelIndex] + originalCoefficientCount - AVG_NDIFF);
    model->m_iCount1[modelIndex] = JxrEncoderCbpPredictorClampModelCount(
        model->m_iCount1[modelIndex] + 16 - originalCoefficientCount - AVG_NDIFF);

    if (model->m_iCount0[modelIndex] < 0) {
        model->m_iState[modelIndex] =
            model->m_iCount0[modelIndex] < model->m_iCount1[modelIndex] ? 1 : 2;
    }
    else if (model->m_iCount1[modelIndex] < 0) {
        model->m_iState[modelIndex] = 2;
    }
    else {
        model->m_iState[modelIndex] = 0;
    }
}

Int JxrEncoderCbpPredictorCalculate(const PixelI* coefficients, Int baseIndex,
    const Int* blockOffsets, Int blockCount, Int flcBits)
{
    U32 threshold = (1u << flcBits) - 1u;
    U32 intervalWidth = threshold * 2u + 1u;
    Int cbp = 0;
    Int block;
    Int coefficient;
    for (block = 0; block < blockCount; ++block) {
        for (coefficient = 1; coefficient < 16; ++coefficient) {
            /* Unsigned addition is modulo 2^32, including negative samples. */
            U32 shifted = (U32)coefficients[baseIndex + blockOffsets[block] + coefficient]
                + threshold;
            if (shifted >= intervalWidth) {
                cbp |= 1 << block;
                break;
            }
        }
    }
    return cbp;
}

Int JxrEncoderCbpPredictorPredict(Int cbp, Int blockCount,
    Bool isLeftBoundary, Bool isTopBoundary, Int leftCbp, Int topCbp,
    CCBPModel* model, Int modelIndex)
{
    Int prediction;
    Int differential;
    Int topBit = blockCount == 16 ? 10 : (blockCount == 8 ? 6 : 2);
    Int leftBit = blockCount == 16 ? 5 : 1;
    Int mask = (1 << blockCount) - 1;

    if (isLeftBoundary)
        prediction = isTopBoundary ? 1 : ((topCbp >> topBit) & 1);
    else
        prediction = (leftCbp >> leftBit) & 1;

    if (blockCount == 16) {
        prediction |= (cbp & 0x3300) << 2;
        prediction |= (cbp & 0xcc) << 6;
        prediction |= (cbp & 0x33) << 2;
        prediction |= (cbp & 0x11) << 1;
        prediction |= (cbp & 0x2) << 3;
    }
    else {
        prediction |= (cbp & 0x1) << 1;
        prediction |= (cbp & 0x3) << 2;
        if (blockCount == 8) {
            prediction |= (cbp & 0xc) << 2;
            prediction |= (cbp & 0x30) << 2;
        }
    }

    if (model->m_iState[modelIndex] == 0)
        differential = prediction ^ cbp;
    else if (model->m_iState[modelIndex] == 1)
        differential = cbp;
    else
        differential = cbp ^ mask;

    JxrEncoderCbpPredictorUpdateModel(model, modelIndex,
        NumOnes(cbp) * (16 / blockCount));
    return differential;
}

/* Legacy adapter: buffer ownership and neighbour access stay at this boundary. */
Void JxrEncoderCbpPredictorApply(CWMImageStrCodec* codec, CCodingContext* context)
{
    size_t column = codec->cColumn - 1;
    Int channel;
    for (channel = 0; channel < (Int)codec->m_param.cNumChannels; ++channel) {
        COLORFORMAT format = codec->m_param.cfColorFormat;
        Int modelIndex = channel > 0 ? 1 : 0;
        Int blockCount = channel == 0 ? 16 :
            (format == YUV_420 ? 4 : (format == YUV_422 ? 8 : 16));
        const Int* offsets = blockCount == 4 ? blkOffsetUV :
            (blockCount == 8 ? blkOffsetUV_422 : blkOffset);
        Int leftCbp = 0;
        Int topCbp = 0;
        Int cbp = JxrEncoderCbpPredictorCalculate(codec->pPlane[channel], 0,
            offsets, blockCount, context->m_aModelAC.m_iFlcBits[modelIndex]);

        codec->MBInfo.iCBP[channel] = cbp;
        codec->PredInfo[channel][column].iCBP = cbp;
        if (!codec->m_bCtxLeft)
            leftCbp = codec->PredInfo[channel][column - 1].iCBP;
        else if (!codec->m_bCtxTop)
            topCbp = codec->PredInfoPrevRow[channel][column].iCBP;

        codec->MBInfo.iDiffCBP[channel] = JxrEncoderCbpPredictorPredict(cbp,
            blockCount, codec->m_bCtxLeft, codec->m_bCtxTop, leftCbp, topCbp,
            &context->m_aCBPModel, modelIndex);
    }
}
