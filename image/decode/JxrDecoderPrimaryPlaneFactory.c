#include "JxrDecoderPrimaryPlaneFactory.h"
#include "JxrDecoderBufferRegionLayout.h"
#include "JxrDecoderCodecStateInitializer.h"
#include "decode.h"
#include "perfTimer.h"
#include <stdlib.h>
#include <string.h>

Int JxrDecoderPrimaryPlaneFactoryCreate(const JxrDecoderMemoryLayoutPlan* memoryLayout,
    const CCoreParameters* parameters, const CWMImageStrCodec* templateCodec,
    Bool usesHardTileBoundaries, Bool measuresPerformance,
    CWMImageStrCodec** primaryCodec)
{
    U8* storage;
    CWMImageStrCodec* codec;
    JxrDecoderBufferRegionLayout bufferLayout;

    UNREFERENCED_PARAMETER(measuresPerformance);
    if (primaryCodec != NULL) *primaryCodec = NULL;
    if (memoryLayout == NULL || parameters == NULL || templateCodec == NULL ||
        primaryCodec == NULL || !memoryLayout->allocationIsSafe) return ICERR_ERROR;

    storage = (U8*)malloc(memoryLayout->allocationBytes);
    if (storage == NULL) return WMP_errOutOfMemory;
    memset(storage, 0, memoryLayout->allocationBytes);
    JxrDecoderBufferRegionLayoutInitialize(&bufferLayout, (UINTPTR_T)storage,
        sizeof(*codec), sizeof(CWMDecoderParameters), sizeof(BitIOInfo), memoryLayout);
    codec = (CWMImageStrCodec*)storage;

    PERFTIMER_ONLY(codec->m_fMeasurePerf = measuresPerformance);
    PERFTIMER_NEW(codec->m_fMeasurePerf, &codec->m_ptEndToEndPerf);
    PERFTIMER_NEW(codec->m_fMeasurePerf, &codec->m_ptEncDecPerf);
    PERFTIMER_START(codec->m_fMeasurePerf, codec->m_ptEndToEndPerf);
    PERFTIMER_START(codec->m_fMeasurePerf, codec->m_ptEncDecPerf);
    PERFTIMER_COPYSTARTTIME(codec->m_fMeasurePerf, codec->m_ptEncDecPerf,
        codec->m_ptEndToEndPerf);

    codec->cbChannel = memoryLayout->channelBytes;
    codec->bUseHardTileBoundaries = usesHardTileBoundaries;
    JxrDecoderCodecStateInitializerInitialize(codec, parameters, templateCodec);
    JxrDecoderBufferRegionLayoutBind(codec, storage, &bufferLayout,
        memoryLayout->fullResolutionMacroblockBytes,
        memoryLayout->chromaMacroblockBytes);
    *primaryCodec = codec;
    return ICERR_OK;
}

Void JxrDecoderPrimaryPlaneFactoryRelease(CWMImageStrCodec* primaryCodec)
{
    free(primaryCodec);
}
