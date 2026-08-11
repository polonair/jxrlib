#include "JxrDecoderStreamInitializer.h"

Void JxrDecoderStreamInitializerInit(JxrDecoderStreamInitializer* initializer, Void* context,
    JxrDecoderStreamInitializationStep allocateBitIo,
    JxrDecoderStreamInitializationStep attachInput,
    JxrDecoderStreamInitializationStep readIndexTable)
{
    initializer->context = context;
    initializer->allocateBitIo = allocateBitIo;
    initializer->attachInput = attachInput;
    initializer->readIndexTable = readIndexTable;
}

Int JxrDecoderStreamInitializerRun(JxrDecoderStreamInitializer* initializer)
{
    if (initializer == NULL || initializer->allocateBitIo == NULL ||
        initializer->attachInput == NULL || initializer->readIndexTable == NULL)
    {
        return ICERR_ERROR;
    }
    if (initializer->allocateBitIo(initializer->context) != ICERR_OK) return ICERR_ERROR;
    if (initializer->attachInput(initializer->context) != ICERR_OK) return ICERR_ERROR;
    if (initializer->readIndexTable(initializer->context) != ICERR_OK) return ICERR_ERROR;
    return ICERR_OK;
}
