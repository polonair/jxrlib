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
static Int predCBPCEnc(CWMImageStrCodec *pSC, Int iCBP, size_t mbX, size_t mbY, size_t c, CCBPModel *pModel)
{
    Int iPredCBP = 0, iRetval = 0;
    Int iNOrig = NumOnes(iCBP);

    UNREFERENCED_PARAMETER( mbY );

    /* only top left block pattern is predicted from neighbour */
    if(pSC->m_bCtxLeft) {
        if (pSC->m_bCtxTop) {
            iPredCBP = 1;
        }
        else {
            Int iTopCBP  = (pSC->PredInfoPrevRow[c] + mbX)->iCBP;
            iPredCBP = (iTopCBP >> 10) & 1; // left: top(10) => 0
        }
    }
    else {
        Int iLeftCBP = (pSC->PredInfo[c] + mbX - 1)->iCBP;
        iPredCBP = ((iLeftCBP >> 5) & 1); // left(5) => 0
    }

    iPredCBP |= (iCBP & 0x3300) << 2; // [8 9 12 13]->[10 11 14 15]
    iPredCBP |= (iCBP & 0xcc) << 6; // [2 3 6 7]->[8 9 12 13]
    iPredCBP |= (iCBP & 0x33) << 2; // [0 1 4 5]->[2 3 6 7]
    iPredCBP |= (iCBP & 0x11) << 1; // [0 4]->[1 5]
    iPredCBP |= (iCBP & 0x2) << 3; // [1]->[4]

    if (c) c = 1;
    if (pModel->m_iState[c] == 0) {
        iRetval = iPredCBP ^ iCBP;
    }
    else if (pModel->m_iState[c] == 1) {
        iRetval = iCBP;
    }
    else {
        iRetval = iCBP ^ 0xffff;
    }

    JxrEncoderCbpPredictorUpdateModel(pModel, c, iNOrig);
    return iRetval;
}

static Int predCBPC420Enc(CWMImageStrCodec *pSC, Int iCBP, size_t mbX, size_t mbY, size_t c, CCBPModel *pModel)
{
    Int iPredCBP = 0, iRetval = 0;
    Int iNOrig = NumOnes(iCBP) * 4;

    UNREFERENCED_PARAMETER( mbY );

    /* only top left block pattern is predicted from neighbour */
    if(pSC->m_bCtxLeft) {
        if (pSC->m_bCtxTop) {
            iPredCBP = 1;
        }
        else {
            Int iTopCBP  = (pSC->PredInfoPrevRow[c] + mbX)->iCBP;
            iPredCBP = (iTopCBP >> 2) & 1; // left: top(2) => 0
        }
    }
    else {
        Int iLeftCBP = (pSC->PredInfo[c] + mbX - 1)->iCBP;
        iPredCBP = ((iLeftCBP >> 1) & 1); // left(1) => 0
    }

    iPredCBP |= (iCBP & 0x1) << 1; // [0]->[1]
    iPredCBP |= (iCBP & 0x3) << 2; // [0 1]->[2 3]

    if (pModel->m_iState[1] == 0) {
        iRetval = iPredCBP ^ iCBP;
    }
    else if (pModel->m_iState[1] == 1) {
        iRetval = iCBP;
    }
    else {
        iRetval = iCBP ^ 0xf;
    }

    JxrEncoderCbpPredictorUpdateModel(pModel, 1, iNOrig);
    return iRetval;
}

static Int predCBPC422Enc(CWMImageStrCodec *pSC, Int iCBP, size_t mbX, size_t mbY, size_t c, CCBPModel *pModel)
{
    Int iPredCBP = 0, iRetval = 0;
    Int iNOrig = NumOnes(iCBP) * 2;

    UNREFERENCED_PARAMETER( mbY );

    /* only top left block pattern is predicted from neighbour */
    if(pSC->m_bCtxLeft) {
        if (pSC->m_bCtxTop) {
            iPredCBP = 1;
        }
        else {
            Int iTopCBP  = (pSC->PredInfoPrevRow[c] + mbX)->iCBP;
            iPredCBP = (iTopCBP >> 6) & 1; // left: top(6) => 0
        }
    }
    else {
        Int iLeftCBP = (pSC->PredInfo[c] + mbX - 1)->iCBP;
        iPredCBP = ((iLeftCBP >> 1) & 1); // left(1) => 0
    }

    iPredCBP |= (iCBP & 0x1) << 1; // [0]->[1]
    iPredCBP |= (iCBP & 0x3) << 2; // [0 1]->[2 3]
    iPredCBP |= (iCBP & 0xc) << 2; // [2 3]->[4 5]
    iPredCBP |= (iCBP & 0x30) << 2; // [4 5]->[6 7]

    if (pModel->m_iState[1] == 0) {
        iRetval = iPredCBP ^ iCBP;
    }
    else if (pModel->m_iState[1] == 1) {
        iRetval = iCBP;
    }
    else {
        iRetval = iCBP ^ 0xff;
    }

    JxrEncoderCbpPredictorUpdateModel(pModel, 1, iNOrig);
    return iRetval;
}

Void JxrEncoderCbpPredictorApply(CWMImageStrCodec* pSC, CCodingContext *pContext)
{
    size_t mbX = pSC->cColumn - 1, mbY = pSC->cRow - 1;
    CWMIMBInfo * pMBInfo = &(pSC->MBInfo);
    int iChannel, i, j;

    for(iChannel = 0; iChannel < (int)pSC->m_param.cNumChannels; iChannel ++){
        const COLORFORMAT cf = pSC->m_param.cfColorFormat;
        const Bool bUV = (iChannel > 0);
        const int iNumBlock = (bUV ? (cf == YUV_422 ? 8 : (cf == YUV_420 ? 4 : 16)) : 16);
        const int * pOffset = (iNumBlock == 4 ? blkOffsetUV : (iNumBlock == 8 ? blkOffsetUV_422 : blkOffset));
        const Int threshold = (1 << pContext->m_aModelAC.m_iFlcBits[bUV ? 1 : 0]) - 1, threshold2 = threshold * 2 + 1;
        Int iCBP = 0;

        for(j = 0; j < iNumBlock; j ++){
            PixelI * pData = pSC->pPlane[iChannel] + pOffset[j];
            for(i = 1; i < 16; i ++){
                if((unsigned int)(pData[i] + threshold) >= (unsigned int) threshold2){ // significant coeff
                    iCBP |= (1 << j); // update CBP
                    break;
                }
            }
        }

        pMBInfo->iCBP[iChannel] = (pSC->PredInfo[iChannel] + mbX)->iCBP = iCBP;

        if(iNumBlock == 16){
            pMBInfo->iDiffCBP[iChannel] = predCBPCEnc(pSC, pMBInfo->iCBP[iChannel], mbX, mbY, iChannel, &pContext->m_aCBPModel);
        }
        else if(iNumBlock == 8){
            pSC->MBInfo.iDiffCBP[iChannel] = predCBPC422Enc(pSC, pMBInfo->iCBP[iChannel], mbX, mbY, iChannel, &pContext->m_aCBPModel);
        }
        else{
            pSC->MBInfo.iDiffCBP[iChannel] = predCBPC420Enc(pSC, pMBInfo->iCBP[iChannel], mbX, mbY, iChannel, &pContext->m_aCBPModel);
        }
    }
}
