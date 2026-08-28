#include "JxrEncoderSessionReleaser.h"
#include "JxrEncoderMacroblockProcessingPipeline.h"
#include "JxrEncoderResourceRelease.h"
#include "perfTimer.h"

Int JxrEncoderSessionReleaserRelease(CWMImageStrCodec* codec)
{
    if (sizeof(*codec) != codec->cbStruct)
        return ICERR_ERROR;

    PERFTIMER_START(codec->m_fMeasurePerf, codec->m_ptEncDecPerf);
    JxrEncoderMacroblockProcessingPipelineProcessFinalRow(codec);
    JxrEncoderResourceReleaseRelease(codec);
    PERFTIMER_STOP(codec->m_fMeasurePerf, codec->m_ptEncDecPerf);
    PERFTIMER_STOP(codec->m_fMeasurePerf, codec->m_ptEndToEndPerf);
    PERFTIMER_REPORT(codec->m_fMeasurePerf, codec);
    PERFTIMER_DELETE(codec->m_fMeasurePerf, codec->m_ptEncDecPerf);
    PERFTIMER_DELETE(codec->m_fMeasurePerf, codec->m_ptEndToEndPerf);
    free(codec);
    return ICERR_OK;
}
