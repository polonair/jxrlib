#include "JxrEncoderSessionFactory.h"
#include "JxrEncoderSessionInitializer.h"
#include "JxrEncoderMemoryLayoutPlan.h"
#include "JxrEncoderBufferRegionLayout.h"
#include "JxrEncoderLegacyContract.h"
#include "encode.h"
Int JxrEncoderSessionFactoryCreate(
    CWMImageInfo* pII,
    CWMIStrCodecParam *pSCP,
    CTXSTRCODEC* pctxSC)
{
    static size_t cbChannels[BD_MAX] = {2, 4};

    size_t cbChannel = 0, cblkChroma = 0;
    size_t cbMacBlockStride = 0, cbMacBlockChroma = 0;

    CWMImageStrCodec* pSC = NULL, *pNextSC = NULL;
    char* pb = NULL;
    size_t cb = 0;
    JxrEncoderMemoryLayoutPlan memoryLayout;
    JxrEncoderBufferRegionLayout primaryBufferRegions;
    JxrEncoderBufferRegionLayout secondaryBufferRegions;

    Int err;

    if(ValidateArgs(pII, pSCP) != ICERR_OK){
        goto ErrorExit;
    }

    //================================================
    *pctxSC = NULL;

    //================================================
    cbChannel = cbChannels[pSCP->bdBitDepth];
    cblkChroma = cblkChromas[pSCP->cfColorFormat];
    JxrEncoderMemoryLayoutPlanInitialize(&memoryLayout, cbChannel, cblkChroma,
        pSCP->cChannel, pII->cWidth, sizeof(*pSC), sizeof(*pSC->pIOHeader),
        sizeof(size_t) == 4);
    if (!memoryLayout.allocationIsSafe)
        return ICERR_ERROR;
    cbMacBlockStride = memoryLayout.fullResolutionMacroblockBytes;
    cbMacBlockChroma = memoryLayout.chromaMacroblockBytes;

    //================================================
    cb = memoryLayout.primaryAllocationBytes;

    pb = malloc(cb);
    if (NULL == pb)
    {
        goto ErrorExit;
    }
    memset(pb, 0, cb);

    //================================================
    pSC = (CWMImageStrCodec*)pb;
    JxrEncoderBufferRegionLayoutInitialize(&primaryBufferRegions, (UINTPTR_T)pSC,
        sizeof(*pSC), &memoryLayout);

    // Set up perf timers
    PERFTIMER_ONLY(pSC->m_fMeasurePerf = pSCP->fMeasurePerf);
    PERFTIMER_NEW(pSC->m_fMeasurePerf, &pSC->m_ptEndToEndPerf);
    PERFTIMER_NEW(pSC->m_fMeasurePerf, &pSC->m_ptEncDecPerf);
    PERFTIMER_START(pSC->m_fMeasurePerf, pSC->m_ptEndToEndPerf);
    PERFTIMER_START(pSC->m_fMeasurePerf, pSC->m_ptEncDecPerf);
    PERFTIMER_COPYSTARTTIME(pSC->m_fMeasurePerf, pSC->m_ptEncDecPerf, pSC->m_ptEndToEndPerf);

    pSC->m_param.cfColorFormat = pSCP->cfColorFormat;
    pSC->m_param.bAlphaChannel = (pSCP->uAlphaMode == 3);
    pSC->m_param.cNumChannels = pSCP->cChannel;
    pSC->m_param.cExtraPixelsTop = pSC->m_param.cExtraPixelsBottom
        = pSC->m_param.cExtraPixelsLeft = pSC->m_param.cExtraPixelsRight = 0;

    pSC->cbChannel = cbChannel;

    pSC->m_param.bTranscode = pSC->bTileExtraction = FALSE;

    //================================================
    JxrEncoderSessionInitializerInitialize(pSC, pII, pSCP);

    //================================================
    // Bind two macroblock rows per channel and the packet/header I/O region.
    JxrEncoderBufferRegionLayoutBindPrimary(pSC, (U8*)pSC, &primaryBufferRegions,
        cbMacBlockStride, cbMacBlockChroma);

    //================================================
    err = StrEncInit(pSC);
    if (ICERR_OK != err)
        goto ErrorExit;

    // if interleaved alpha is needed
    if (pSC->m_param.bAlphaChannel) {
        cbMacBlockStride = memoryLayout.fullResolutionMacroblockBytes;
        // 1. allocate new pNextSC info
        //================================================
        cb = memoryLayout.secondaryAllocationBytes;
        pb = malloc(cb);
        if (NULL == pb)
        {
            goto ErrorExit;
        }
        memset(pb, 0, cb);
        //================================================
        pNextSC = (CWMImageStrCodec*)pb;
        JxrEncoderBufferRegionLayoutInitialize(&secondaryBufferRegions, (UINTPTR_T)pNextSC,
            sizeof(*pNextSC), &memoryLayout);

        // 2. initialize pNextSC
        pNextSC->m_param.cfColorFormat = Y_ONLY;
        pNextSC->m_param.cNumChannels = 1;
        pNextSC->m_param.bAlphaChannel = TRUE;
        pNextSC->cbChannel = cbChannel;
        //================================================

        // 3. initialize arrays
        JxrEncoderSessionInitializerInitialize(pNextSC, pII, pSCP);
        //================================================

        // Bind two macroblock rows for the secondary alpha plane.
        JxrEncoderBufferRegionLayoutBindSecondary(pNextSC, (U8*)pNextSC,
            &secondaryBufferRegions, cbMacBlockStride);
        //================================================
        pNextSC->pIOHeader = pSC->pIOHeader;
        //================================================

        // 4. link pSC->pNextSC = pNextSC
        pNextSC->m_pNextSC = pSC;
        pNextSC->m_bSecondary = TRUE;

        // 5. StrEncInit
        StrEncInit(pNextSC);

        // 6. Write header of image plane
        WriteImagePlaneHeader(pNextSC);
    }

    pSC->m_pNextSC = pNextSC;
    //================================================
    *pctxSC = (CTXSTRCODEC)pSC;

    writeIndexTableNull(pSC);
#if defined(WMP_OPT_SSE2) || defined(WMP_OPT_CC_ENC) || defined(WMP_OPT_TRFM_ENC)
    StrEncOpt(pSC);
#endif // OPT defined

    PERFTIMER_STOP(pSC->m_fMeasurePerf, pSC->m_ptEncDecPerf);
    return ICERR_OK;

ErrorExit:
    return ICERR_ERROR;
}
