#include "JxrDecoderSessionExecutor.h"
#include "JxrDecoderExecutionPipeline.h"
#include "JxrDecoderOutputPipeline.h"
#include "JXRTrace.h"
#include "perfTimer.h"

Int JxrDecoderSessionExecutorExecute(CWMImageStrCodec* codec,
    const CWMImageBufferInfo* outputBuffer
#ifdef REENTRANT_MODE
    , size_t* decodedLines
#endif
    )
{
    JxrDecoderExecutionPreparation preparation;

    if (codec == NULL || sizeof(*codec) != codec->cbStruct) return ICERR_ERROR;
    JXRTraceDumpCodecState("decoder", codec);
    PERFTIMER_START(codec->m_fMeasurePerf, codec->m_ptEncDecPerf);
    if (JxrDecoderExecutionPipelinePrepare(codec, outputBuffer, &preparation) != ICERR_OK)
        return ICERR_ERROR;
    if (JxrDecoderExecutionPipelineRun(codec, preparation.macroblockRowCount,
        preparation.usesLegacyLoadCallback
#ifdef REENTRANT_MODE
        , decodedLines
#endif
        ) != ICERR_OK)
        return ICERR_ERROR;
#ifndef REENTRANT_MODE
    JxrDecoderOutputPipelineFinalize(codec, outputBuffer);
#endif
    PERFTIMER_STOP(codec->m_fMeasurePerf, codec->m_ptEncDecPerf);
    return ICERR_OK;
}
