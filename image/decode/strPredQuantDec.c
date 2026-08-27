//*@@@+++@@@@******************************************************************
//
// Copyright © Microsoft Corp.
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 
// • Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
// • Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
//*@@@---@@@@******************************************************************

#include "strcodec.h"
#include "JxrCbpPredictor.h"
#include "JxrPredictionMath.h"


/*************************************************************************
    CBP
*************************************************************************/
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


/* CBP prediction for 16 x 16 MB */
/* block index */
/*  0  1  4  5 */
/*  2  3  6  7 */
/*  8  9 12 13 */
/* 10 11 14 15 */
static Int JxrCbpPredictorGetCurrentCbp(CWMImageStrCodec* codec, size_t channel, size_t column)
{
    return codec->PredInfo[channel][column].iCBP;
}

static Int JxrCbpPredictorGetPreviousCbp(CWMImageStrCodec* codec, size_t channel, size_t column)
{
    return codec->PredInfoPrevRow[channel][column].iCBP;
}

static Void JxrCbpPredictorSetCurrentCbp(CWMImageStrCodec* codec, size_t channel,
    size_t column, Int cbp)
{
    codec->PredInfo[channel][column].iCBP = cbp;
}
static Int predCBPCDec(CWMImageStrCodec * pSC, Int iCBP, size_t mbX, size_t mbY, size_t c, CCBPModel *pModel)
{
    Int iNOrig;
    const int iNDiff = AVG_NDIFF;
    size_t c1 = c ? 1 : 0;

    UNREFERENCED_PARAMETER( mbY );

    if (pModel->m_iState[c1] == 0) {
        if(pSC->m_bCtxLeft) {
            if (pSC->m_bCtxTop) {
                iCBP ^= 1;
            }
            else {
                Int iTopCBP  = JxrCbpPredictorGetPreviousCbp(pSC, c, mbX);
                iCBP ^= (iTopCBP >> 10) & 1; // left: top(10) => 0
            }
        }
        else {
            Int iLeftCBP = JxrCbpPredictorGetCurrentCbp(pSC, c, mbX - 1);
            iCBP ^= ((iLeftCBP >> 5) & 1); // left(5) => 0
        }

        iCBP ^= (0x02 & (iCBP << 1)); // 0 => 1
        iCBP ^= (0x10 & (iCBP << 3)); // 1 => 4
        iCBP ^= (0x20 & (iCBP << 1)); // 4 => 5

        iCBP ^= ((iCBP & 0x33) << 2);
        iCBP ^= ((iCBP & 0xcc) << 6);
        iCBP ^= ((iCBP & 0x3300) << 2);

    }
    else if (pModel->m_iState[c1] == 2) {
        iCBP ^= 0xffff;
    }

    iNOrig = NumOnes(iCBP);

    pModel->m_iCount0[c1] += iNOrig - iNDiff;
    pModel->m_iCount0[c1] = JxrPredictionMathSaturateAdaptiveCount(pModel->m_iCount0[c1]);

    pModel->m_iCount1[c1] += 16 - iNOrig - iNDiff;
    pModel->m_iCount1[c1] = JxrPredictionMathSaturateAdaptiveCount(pModel->m_iCount1[c1]);

    if (pModel->m_iCount0[c1] < 0) {
        if (pModel->m_iCount0[c1] < pModel->m_iCount1[c1]) {
            pModel->m_iState[c1] = 1;
        }
        else {
            pModel->m_iState[c1] = 2;
        }
    }
    else if (pModel->m_iCount1[c1] < 0) {
        pModel->m_iState[c1] = 2;
    }
    else {
        pModel->m_iState[c1] = 0;
    }
    return iCBP;
}

static Int predCBPC420Dec(CWMImageStrCodec * pSC, Int iCBP, size_t mbX, size_t mbY, size_t c, CCBPModel *pModel)
{
    Int iNOrig;
    const int iNDiff = AVG_NDIFF;

    UNREFERENCED_PARAMETER( mbY );

    if (pModel->m_iState[1] == 0) {
        if(pSC->m_bCtxLeft) {
            if (pSC->m_bCtxTop) {
                iCBP ^= 1;
            }
            else {
                Int iTopCBP  = JxrCbpPredictorGetPreviousCbp(pSC, c, mbX);
                iCBP ^= (iTopCBP >> 2) & 1; // left: top(2) => 0
            }
        }
        else {
            Int iLeftCBP = JxrCbpPredictorGetCurrentCbp(pSC, c, mbX - 1);
            iCBP ^= ((iLeftCBP >> 1) & 1); // left(1) => 0
        }

        iCBP ^= (0x02 & (iCBP << 1)); // 0 => 1
        iCBP ^= ((iCBP & 0x3) << 2); // [0 1] -> [2 3]
    }
    else if (pModel->m_iState[1] == 2) {
        iCBP ^= 0xf;
    }

    iNOrig = NumOnes(iCBP) * 4;

    pModel->m_iCount0[1] += iNOrig - iNDiff;
    pModel->m_iCount0[1] = JxrPredictionMathSaturateAdaptiveCount(pModel->m_iCount0[1]);

    pModel->m_iCount1[1] += 16 - iNOrig - iNDiff;
    pModel->m_iCount1[1] = JxrPredictionMathSaturateAdaptiveCount(pModel->m_iCount1[1]);

    if (pModel->m_iCount0[1] < 0) {
        if (pModel->m_iCount0[1] < pModel->m_iCount1[1]) {
            pModel->m_iState[1] = 1;
        }
        else {
            pModel->m_iState[1] = 2;
        }
    }
    else if (pModel->m_iCount1[1] < 0) {
        pModel->m_iState[1] = 2;
    }
    else {
        pModel->m_iState[1] = 0;
    }

    return iCBP;
}

static Int predCBPC422Dec(CWMImageStrCodec * pSC, Int iCBP, size_t mbX, size_t mbY, size_t c, CCBPModel *pModel)
{
    Int iNOrig;
    const int iNDiff = AVG_NDIFF;

    UNREFERENCED_PARAMETER( mbY );

    if (pModel->m_iState[1] == 0) {
        if(pSC->m_bCtxLeft) {
            if (pSC->m_bCtxTop) {
                iCBP ^= 1;
            }
            else {
                Int iTopCBP  = JxrCbpPredictorGetPreviousCbp(pSC, c, mbX);
                iCBP ^= (iTopCBP >> 6) & 1; // left: top(6) => 0
            }
        }
        else {
            Int iLeftCBP = JxrCbpPredictorGetCurrentCbp(pSC, c, mbX - 1);
            iCBP ^= ((iLeftCBP >> 1) & 1); // left(1) => 0
        }
        
        iCBP ^= (iCBP & 0x1) << 1; // [0]->[1]
        iCBP ^= (iCBP & 0x3) << 2; // [0 1]->[2 3]
        iCBP ^= (iCBP & 0xc) << 2; // [2 3]->[4 5]
        iCBP ^= (iCBP & 0x30) << 2; // [4 5]->[6 7]
    }
    else if (pModel->m_iState[1] == 2) {
        iCBP ^= 0xff;
    }

    iNOrig = NumOnes(iCBP) * 2;

    pModel->m_iCount0[1] += iNOrig - iNDiff;
    pModel->m_iCount0[1] = JxrPredictionMathSaturateAdaptiveCount(pModel->m_iCount0[1]);

    pModel->m_iCount1[1] += 16 - iNOrig - iNDiff;
    pModel->m_iCount1[1] = JxrPredictionMathSaturateAdaptiveCount(pModel->m_iCount1[1]);

    if (pModel->m_iCount0[1] < 0) {
        if (pModel->m_iCount0[1] < pModel->m_iCount1[1]) {
            pModel->m_iState[1] = 1;
        }
        else {
            pModel->m_iState[1] = 2;
        }
    }
    else if (pModel->m_iCount1[1] < 0) {
        pModel->m_iState[1] = 2;
    }
    else {
        pModel->m_iState[1] = 0;
    }

    return iCBP;
}


/* Coded Block Pattern (CBP) prediction */
Void JxrCbpPredictorDecode(JxrDecoderSubbandContext* state)
{
    CWMImageStrCodec* codec = state->codec;
    const COLORFORMAT cf = codec->m_param.cfColorFormat;
    const size_t channelCount = (cf == YUV_420 || cf == YUV_422) ? 1 : codec->m_param.cNumChannels;
    size_t channel;
    const size_t macroblockX = codec->cColumn;
    const size_t macroblockY = codec->cRow;

    for (channel = 0; channel < channelCount; ++channel) {
        Int cbp = predCBPCDec(codec, JxrMacroblockCbpStateGetDifferential(&state->macroblockCbpState, (Int)channel), macroblockX, macroblockY,
            channel, JxrHighpassCbpStateGetPredictionModel(&state->highpassCbpState));
        JxrMacroblockCbpStateSetCbp(&state->macroblockCbpState, (Int)channel, cbp);
        JxrCbpPredictorSetCurrentCbp(codec, channel, macroblockX, cbp);
    }

    if (cf == YUV_422) {
        Int cbpU = predCBPC422Dec(codec, JxrMacroblockCbpStateGetDifferential(&state->macroblockCbpState, 1), macroblockX, macroblockY,
            1, JxrHighpassCbpStateGetPredictionModel(&state->highpassCbpState));
        Int cbpV = predCBPC422Dec(codec, JxrMacroblockCbpStateGetDifferential(&state->macroblockCbpState, 2), macroblockX, macroblockY,
            2, JxrHighpassCbpStateGetPredictionModel(&state->highpassCbpState));
        JxrMacroblockCbpStateSetCbp(&state->macroblockCbpState, 1, cbpU);
        JxrMacroblockCbpStateSetCbp(&state->macroblockCbpState, 2, cbpV);
        JxrCbpPredictorSetCurrentCbp(codec, 1, macroblockX, cbpU);
        JxrCbpPredictorSetCurrentCbp(codec, 2, macroblockX, cbpV);
    }
    else if (cf == YUV_420) {
        Int cbpU = predCBPC420Dec(codec, JxrMacroblockCbpStateGetDifferential(&state->macroblockCbpState, 1), macroblockX, macroblockY,
            1, JxrHighpassCbpStateGetPredictionModel(&state->highpassCbpState));
        Int cbpV = predCBPC420Dec(codec, JxrMacroblockCbpStateGetDifferential(&state->macroblockCbpState, 2), macroblockX, macroblockY,
            2, JxrHighpassCbpStateGetPredictionModel(&state->highpassCbpState));
        JxrMacroblockCbpStateSetCbp(&state->macroblockCbpState, 1, cbpU);
        JxrMacroblockCbpStateSetCbp(&state->macroblockCbpState, 2, cbpV);
        JxrCbpPredictorSetCurrentCbp(codec, 1, macroblockX, cbpU);
        JxrCbpPredictorSetCurrentCbp(codec, 2, macroblockX, cbpV);
    }
}

Void predCBPDec(CWMImageStrCodec* codec, CCodingContext* entropy)
{
    JxrDecoderSubbandContext state;
    JxrDecoderSubbandContextInit(&state, codec, entropy);
    JxrCbpPredictorDecode(&state);
}