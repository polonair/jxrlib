#include "JxrDecoderInitializationPipeline.h"

Void JxrDecoderInitializationPipelineInit(JxrDecoderInitializationPipeline* pipeline,
    CWMImageStrCodec* primaryCodec, CWMImageStrCodec* secondaryCodec,
    JxrDecoderInitializeStage initializeIo, JxrDecoderInitializeStage initializeDecoder)
{
    pipeline->primaryCodec = primaryCodec;
    pipeline->secondaryCodec = secondaryCodec;
    pipeline->initializeIo = initializeIo;
    pipeline->initializeDecoder = initializeDecoder;
}

Int JxrDecoderInitializationPipelineRun(JxrDecoderInitializationPipeline* pipeline)
{
    if (pipeline == NULL || pipeline->primaryCodec == NULL ||
        pipeline->initializeIo == NULL || pipeline->initializeDecoder == NULL ||
        pipeline->initializeIo(pipeline->primaryCodec) != ICERR_OK ||
        pipeline->initializeDecoder(pipeline->primaryCodec) != ICERR_OK ||
        (pipeline->secondaryCodec != NULL &&
        pipeline->initializeDecoder(pipeline->secondaryCodec) != ICERR_OK))
        return ICERR_ERROR;
    pipeline->primaryCodec->m_pNextSC = pipeline->secondaryCodec;
    return ICERR_OK;
}
