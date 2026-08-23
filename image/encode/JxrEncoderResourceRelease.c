#include "JxrEncoderResourceRelease.h"
#include "encode.h"
#include <stdlib.h>

/* Defined by the remaining encoder stream facade in strenc.c. */
Int StrIOEncTerm(CWMImageStrCodec* codec);

Void JxrEncoderResourceReleasePlanInitialize(JxrEncoderResourceReleasePlan* plan,
    size_t codecIndex, Bool changesUvResolution)
{
    plan->releasesChromaResiduals = changesUvResolution;
    plan->finalizesPrimaryOutput = codecIndex == 0;
    plan->releasesPredictionState = TRUE;
    plan->releasesCodingContexts = TRUE;
    plan->releasesTileState = TRUE;
}

Int JxrEncoderResourceReleaseRelease(CWMImageStrCodec* codec)
{
    CWMImageStrCodec* currentCodec = codec;
    size_t codecIndex;
    size_t codecCount = codec->m_pNextSC != NULL ? 2 : 1;

    for (codecIndex = 0; codecIndex < codecCount; ++codecIndex) {
        JxrEncoderResourceReleasePlan plan;

        if (sizeof(*currentCodec) != currentCodec->cbStruct)
            return ICERR_ERROR;

        JxrEncoderResourceReleasePlanInitialize(&plan, codecIndex,
            currentCodec->m_bUVResolutionChange);
        if (plan.releasesChromaResiduals) {
            if (currentCodec->pResU != NULL)
                free(currentCodec->pResU);
            if (currentCodec->pResV != NULL)
                free(currentCodec->pResV);
        }

        if (plan.releasesPredictionState)
            freePredInfo(currentCodec);

        /* Preserve legacy behavior: stream-finalization errors are ignored here. */
        if (plan.finalizesPrimaryOutput)
            StrIOEncTerm(currentCodec);

        if (plan.releasesCodingContexts)
            FreeCodingContextEnc(currentCodec);
        if (plan.releasesTileState)
            freeTileInfo(currentCodec);

        currentCodec->WMISCP.nExpBias -= 128;
        currentCodec = currentCodec->m_pNextSC;
    }

    return ICERR_OK;
}
