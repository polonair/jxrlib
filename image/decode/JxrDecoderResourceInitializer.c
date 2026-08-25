#include "JxrDecoderResourceInitializer.h"

Int JxrDecoderResourceInitializerInitialize(CWMImageStrCodec* pSC)
{
    // CWMImageInfo * pWMII = &pSC->WMII;
    COLORFORMAT cfInt = pSC->m_param.cfColorFormat;
    COLORFORMAT cfExt = pSC->WMII.cfColorFormat;
    size_t i;

    /** color transcoding with resolution change **/
    pSC->m_bUVResolutionChange = ((cfExt != Y_ONLY) && ((cfInt == YUV_420 && cfExt != YUV_420) ||
        (cfInt == YUV_422 && cfExt != YUV_422))) && !pSC->WMISCP.bYUVData;
    if(pSC->m_bUVResolutionChange){
        pSC->pResU = (PixelI *)malloc((cfExt == YUV_422 ? 128 : 256) * pSC->cmbWidth * sizeof(PixelI));
        pSC->pResV = (PixelI *)malloc((cfExt == YUV_422 ? 128 : 256) * pSC->cmbWidth * sizeof(PixelI));
        if(pSC->pResU == NULL || pSC->pResV == NULL || (cfExt == YUV_422 ? 128 : 256) * pSC->cmbWidth * sizeof(PixelI) < pSC->cmbWidth){
            return ICERR_ERROR;
        }
    }

    if(allocatePredInfo(pSC) != ICERR_OK){
        return ICERR_ERROR;
    }

    if(allocateTileInfo(pSC) != ICERR_OK)
        return ICERR_ERROR;

    if((pSC->m_param.uQPMode & 1) == 0){ // DC frame uniform quantization
        if(allocateQuantizer(pSC->pTile[0].pQuantizerDC, pSC->m_param.cNumChannels, 1) != ICERR_OK)
            return ICERR_ERROR;
        setUniformQuantizer(pSC, 0);
        for(i = 0; i < pSC->m_param.cNumChannels; i ++)
            pSC->pTile[0].pQuantizerDC[i]->iIndex = pSC->m_param.uiQPIndexDC[i];
        formatQuantizer(pSC->pTile[0].pQuantizerDC, (pSC->m_param.uQPMode >> 3) & 3, pSC->m_param.cNumChannels, 0, TRUE, pSC->m_param.bScaledArith);
    }

    if(pSC->WMISCP.sbSubband != SB_DC_ONLY){
        if((pSC->m_param.uQPMode & 2) == 0){ // LP frame uniform quantization
            if(allocateQuantizer(pSC->pTile[0].pQuantizerLP, pSC->m_param.cNumChannels, 1) != ICERR_OK)
                return ICERR_ERROR;
            setUniformQuantizer(pSC, 1);
            if((pSC->m_param.uQPMode & 0x200) == 0) // use DC quantizer
                useDCQuantizer(pSC, 0);
            else{
                for(i = 0; i < pSC->m_param.cNumChannels; i ++)
                    pSC->pTile[0].pQuantizerLP[i]->iIndex = pSC->m_param.uiQPIndexLP[i];
                formatQuantizer(pSC->pTile[0].pQuantizerLP, (pSC->m_param.uQPMode >> 5) & 3, pSC->m_param.cNumChannels, 0, TRUE, pSC->m_param.bScaledArith);
            }
        }

        if(pSC->WMISCP.sbSubband != SB_NO_HIGHPASS){
            if((pSC->m_param.uQPMode & 4) == 0){ // HP frame uniform quantization
                if(allocateQuantizer(pSC->pTile[0].pQuantizerHP, pSC->m_param.cNumChannels, 1) != ICERR_OK)
                    return ICERR_ERROR;
                setUniformQuantizer(pSC, 2);

                if((pSC->m_param.uQPMode & 0x400) == 0) // use LP quantizer
                    useLPQuantizer(pSC, 1, 0);
                else{
                    for(i = 0; i < pSC->m_param.cNumChannels; i ++)
                        pSC->pTile[0].pQuantizerHP[i]->iIndex = pSC->m_param.uiQPIndexHP[i];
                    formatQuantizer(pSC->pTile[0].pQuantizerHP, (pSC->m_param.uQPMode >> 7) & 3, pSC->m_param.cNumChannels, 0, FALSE, pSC->m_param.bScaledArith);
                }
            }
        }
    }

    if(pSC->WMISCP.cNumOfSliceMinus1V >= MAX_TILES || AllocateCodingContextDec(pSC, pSC->WMISCP.cNumOfSliceMinus1V + 1) != ICERR_OK){
        return ICERR_ERROR;
    }

    if (pSC->m_bSecondary) {
        pSC->pIOHeader = pSC->m_pNextSC->pIOHeader;
        pSC->m_ppBitIO = pSC->m_pNextSC->m_ppBitIO;
        pSC->cNumBitIO = pSC->m_pNextSC->cNumBitIO;
        pSC->cSB = pSC->m_pNextSC->cSB;
    }

    setBitIOPointers(pSC);

    return ICERR_OK;
}