#ifndef JXR_DECODER_STREAM_INITIALIZER_H
#define JXR_DECODER_STREAM_INITIALIZER_H

#include "strcodec.h"

typedef Int (*JxrDecoderStreamInitializationStep)(Void* context);

typedef struct JxrDecoderStreamInitializer {
    Void* context;
    JxrDecoderStreamInitializationStep allocateBitIo;
    JxrDecoderStreamInitializationStep attachInput;
    JxrDecoderStreamInitializationStep readIndexTable;
} JxrDecoderStreamInitializer;

Void JxrDecoderStreamInitializerInit(JxrDecoderStreamInitializer* initializer, Void* context,
    JxrDecoderStreamInitializationStep allocateBitIo,
    JxrDecoderStreamInitializationStep attachInput,
    JxrDecoderStreamInitializationStep readIndexTable);
Int JxrDecoderStreamInitializerRun(JxrDecoderStreamInitializer* initializer);

#endif
