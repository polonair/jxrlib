#include "JxrEncoderCoefficientPredictor.h"
#include "encode.h"

/* frequency domain prediction */
Void JxrEncoderCoefficientPredictorApply(CWMImageStrCodec * pSC)
{
    const COLORFORMAT cf = pSC->m_param.cfColorFormat;
    const Int iChannels = (cf == YUV_420 || cf == YUV_422) ? 1 : (Int) pSC->m_param.cNumChannels;
    size_t mbX = pSC->cColumn - 1;// mbY = pSC->cRow - 1;
    CWMIMBInfo *pMBInfo = &(pSC->MBInfo);
    Int iDCACPredMode = getDCACPredMode(pSC, mbX);
    Int iDCPredMode = (iDCACPredMode & 0x3);
    Int iADPredMode = (iDCACPredMode & 0xC);
    Int iACPredMode = getACPredMode(pMBInfo, cf);
    PixelI * pOrg, * pRef;
    Int i, j, k;

    pMBInfo->iOrientation = 2 - iACPredMode;

    /* keep necessary info for future prediction */
    updatePredInfo(pSC, pMBInfo, mbX, cf);

    for(i = 0; i < iChannels; i ++){
        pOrg = pMBInfo->iBlockDC[i]; // current DC block

        /* DC prediction */
        if(iDCPredMode == 1){ // predict DC from top
            pOrg[0] -= (pSC->PredInfoPrevRow[i] + mbX)->iDC;
        }
        else if(iDCPredMode == 0){ // predict DC from left
            pOrg[0] -= (pSC->PredInfo[i] + mbX - 1)->iDC;
        }
        else if(iDCPredMode == 2){// predict DC from top&left
            pOrg[0] -= ((pSC->PredInfo[i] + mbX - 1)->iDC + (pSC->PredInfoPrevRow[i] + mbX)->iDC) >> 1;
        }

        /* AD prediction */
        if(iADPredMode == 4){// predict AD from top
            pRef = (pSC->PredInfoPrevRow[i] + mbX)->piAD;
            pOrg[4] -= pRef[3], pOrg[8] -= pRef[4], pOrg[12] -= pRef[5];
        }
        else if(iADPredMode == 0){// predict AD from left
            pRef = (pSC->PredInfo[i] + mbX - 1)->piAD;
            pOrg[1] -= pRef[0], pOrg[2] -= pRef[1], pOrg[3] -= pRef[2];
        }

        pOrg = pSC->pPlane[i];
        /* AC prediction */
        if(iACPredMode == 1){ // predict from top
            for(k = 0; k <= 192; k += 64){
                /* inside macroblock, in reverse order */
                for(j = 48; j > 0; j -= 16){
                    pOrg[k + j + 10] -= pOrg[k + j + 10 - 16];
                    pOrg[k + j +  2] -= pOrg[k + j +  2 - 16];
                    pOrg[k + j +  9] -= pOrg[k + j +  9 - 16];
                }
            }
        }
        else if(iACPredMode == 0){ // predict from left
            for(k = 0; k < 64; k += 16){
                /* inside macroblock, in reverse order */
                for(j = 192; j > 0; j -= 64){
                    pOrg[k + j + 5] -= pOrg[k + j + 5 - 64];
                    pOrg[k + j + 1] -= pOrg[k + j + 1 - 64];
                    pOrg[k + j + 6] -= pOrg[k + j + 6 - 64];
                }
            }
        }
    }

    if(cf == YUV_420){
        for(i = 1; i < 3; i ++){
            pOrg = pMBInfo->iBlockDC[i]; // current DC block

            /* DC prediciton */
            if(iDCPredMode == 1){ // predict DC from top
                pOrg[0] -= (pSC->PredInfoPrevRow[i] + mbX)->iDC;
            }
            else if(iDCPredMode == 0){ // predict DC from left
                pOrg[0] -= (pSC->PredInfo[i] + mbX - 1)->iDC;
            }
            else if(iDCPredMode == 2){ // predict DC from top&left
                pOrg[0] -= (((pSC->PredInfo[i] + mbX - 1)->iDC + (pSC->PredInfoPrevRow[i] + mbX)->iDC + 1) >> 1);
            }

            /* AD prediction */
            if(iADPredMode == 4){// predict AD from top
                pOrg[2] -= (pSC->PredInfoPrevRow[i] + mbX)->piAD[1];
            }
            else if(iADPredMode == 0){// predict AD from left
                pOrg[1] -= (pSC->PredInfo[i] + mbX - 1)->piAD[0];
            }

            pOrg = pSC->pPlane[i];
            /* AC prediction */
            if(iACPredMode == 1){ // predict from top
                for(j = 16; j <= 48; j += 32){
                    /* inside macroblock */
                    pOrg[j + 10] -= pOrg[j + 10 - 16];
                    pOrg[j +  2] -= pOrg[j +  2 - 16];
                    pOrg[j +  9] -= pOrg[j +  9 - 16];
                }
            }
            else if(iACPredMode == 0){ // predict from left
                for(j = 32; j <= 48; j += 16){
                    /* inside macroblock */
                    pOrg[j + 5] -= pOrg[j + 5 - 32];
                    pOrg[j + 1] -= pOrg[j + 1 - 32];
                    pOrg[j + 6] -= pOrg[j + 6 - 32];
                }
            }
        }
    }
    else if(cf == YUV_422){
        for(i = 1; i < 3; i ++){
            pOrg = pMBInfo->iBlockDC[i]; // current DC block

            /* DC prediciton */
            if(iDCPredMode == 1){ // predict DC from top
                pOrg[0] -= (pSC->PredInfoPrevRow[i] + mbX)->iDC;
            }
            else if(iDCPredMode == 0){ // predict DC from left
                pOrg[0] -= (pSC->PredInfo[i] + mbX - 1)->iDC;
            }
            else if(iDCPredMode == 2){ // predict DC from top&left
                pOrg[0] -= (((pSC->PredInfo[i] + mbX - 1)->iDC + (pSC->PredInfoPrevRow[i] + mbX)->iDC + 1) >> 1);
            }

            /* AD prediction */
            if(iADPredMode == 4){// predict AD from top
                pOrg[4] -= (pSC->PredInfoPrevRow[i] + mbX)->piAD[4]; // AC of HT !!!
                pOrg[6] -= pOrg[2];
                pOrg[2] -= (pSC->PredInfoPrevRow[i] + mbX)->piAD[3];
            }
            else if(iADPredMode == 0){// predict AD from left
                pOrg[4] -= (pSC->PredInfo[i] + mbX - 1)->piAD[4];  // AC of HT !!!
                pOrg[1] -= (pSC->PredInfo[i] + mbX - 1)->piAD[0];
                pOrg[5] -= (pSC->PredInfo[i] + mbX - 1)->piAD[2];
            }
            else if(iDCPredMode == 1){
                pOrg[6] -= pOrg[2];
            }

            pOrg = pSC->pPlane[i]; // current MB
            /* AC prediction */
            if(iACPredMode == 1){ // predict from top
                for(j = 48; j > 0; j -= 16){
                    for(k = 0; k <= 64; k += 64){
                        /* inside macroblock */
                        pOrg[j + k + 10] -= pOrg[j + k + 10 - 16];
                        pOrg[j + k +  2] -= pOrg[j + k +  2 - 16];
                        pOrg[j + k +  9] -= pOrg[j + k +  9 - 16];
                    }
                }
            }
            else if(iACPredMode == 0){ // predict from left
                for(j = 64; j <= 112; j += 16){
                    /* inside macroblock */
                    pOrg[j + 5] -= pOrg[j + 5 - 64];
                    pOrg[j + 1] -= pOrg[j + 1 - 64];
                    pOrg[j + 6] -= pOrg[j + 6 - 64];
                }
            }
        }
    }
}
