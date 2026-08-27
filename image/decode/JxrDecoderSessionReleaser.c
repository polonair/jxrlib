#include "JxrDecoderSessionReleaser.h"
#include "JxrDecoderPrimaryPlaneFactory.h"
#include "JxrDecoderResourceInitializer.h"
#include "JXRTrace.h"
#include "perfTimer.h"

Int JxrDecoderSessionReleaserRelease(CWMImageStrCodec* codec)
{
    if (codec == NULL) return ICERR_OK;
    if (sizeof(*codec) != codec->cbStruct) return ICERR_ERROR;

    JXRTraceDumpCodecState("decoder", codec);
    PERFTIMER_START(codec->m_fMeasurePerf, codec->m_ptEncDecPerf);
    JxrDecoderResourceInitializerRelease(codec);
    PERFTIMER_STOP(codec->m_fMeasurePerf, codec->m_ptEncDecPerf);
    PERFTIMER_REPORT(codec->m_fMeasurePerf, codec);
    PERFTIMER_DELETE(codec->m_fMeasurePerf, codec->m_ptEncDecPerf);
    PERFTIMER_DELETE(codec->m_fMeasurePerf, codec->m_ptEndToEndPerf);
    JxrDecoderPrimaryPlaneFactoryRelease(codec);
    return ICERR_OK;
}
