#include "JxrDecoderCodingContextResetter.h"

Bool JxrDecoderCodingContextResetterResetContexts(
    const JxrDecoderCodingContextResetterConfig* config,
    const JxrDecoderCodingContextResetterOperations* operations)
{
    size_t index;
    size_t resetCount;

    if (config == NULL || operations == NULL || config->contexts == NULL ||
        config->contextCount == 0 || operations->reset == NULL)
        return FALSE;

    resetCount = config->resetAll ? config->contextCount : 1;
    for (index = 0; index < resetCount; ++index)
        operations->reset(operations->context, &config->contexts[index]);
    return TRUE;
}
