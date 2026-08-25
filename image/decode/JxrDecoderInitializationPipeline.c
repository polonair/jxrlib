#include "JxrDecoderInitializationPipeline.h"
#include "JxrDecoderResourceInitializer.h"

Void JxrDecoderInitializationPipelineInit(JxrDecoderInitializationPipeline* pipeline,
    CWMImageStrCodec* primaryCodec, CWMImageStrCodec* secondaryCodec,
    JxrDecoderInitializeStage initializeIo)
{
    pipeline->primaryCodec = primaryCodec;
    pipeline->secondaryCodec = secondaryCodec;
    pipeline->initializeIo = initializeIo;
}

Int JxrDecoderInitializationPipelineRun(JxrDecoderInitializationPipeline* pipeline)
{
    if (pipeline == NULL || pipeline->primaryCodec == NULL ||
        pipeline->initializeIo == NULL ||
        pipeline->initializeIo(pipeline->primaryCodec) != ICERR_OK ||
        JxrDecoderResourceInitializerInitialize(pipeline->primaryCodec) != ICERR_OK ||
        (pipeline->secondaryCodec != NULL &&
        JxrDecoderResourceInitializerInitialize(pipeline->secondaryCodec) != ICERR_OK))
        return ICERR_ERROR;
    pipeline->primaryCodec->m_pNextSC = pipeline->secondaryCodec;
    return ICERR_OK;
}
