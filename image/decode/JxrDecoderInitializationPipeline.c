#include "JxrDecoderInitializationPipeline.h"
#include "JxrDecoderInputInitializer.h"
#include "JxrDecoderResourceInitializer.h"

Void JxrDecoderInitializationPipelineInit(JxrDecoderInitializationPipeline* pipeline,
    CWMImageStrCodec* primaryCodec, CWMImageStrCodec* secondaryCodec)
{
    pipeline->primaryCodec = primaryCodec;
    pipeline->secondaryCodec = secondaryCodec;
}

Int JxrDecoderInitializationPipelineRun(JxrDecoderInitializationPipeline* pipeline)
{
    if (pipeline == NULL || pipeline->primaryCodec == NULL ||
        JxrDecoderInputInitializerInitialize(pipeline->primaryCodec) != ICERR_OK ||
        JxrDecoderResourceInitializerInitialize(pipeline->primaryCodec) != ICERR_OK ||
        (pipeline->secondaryCodec != NULL &&
        JxrDecoderResourceInitializerInitialize(pipeline->secondaryCodec) != ICERR_OK))
        return ICERR_ERROR;
    pipeline->primaryCodec->m_pNextSC = pipeline->secondaryCodec;
    return ICERR_OK;
}
