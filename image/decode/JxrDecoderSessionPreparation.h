#ifndef JXR_DECODER_SESSION_PREPARATION_H
#define JXR_DECODER_SESSION_PREPARATION_H

#include "strcodec.h"

/* Validated public input plus the template state used to create decoder planes. */
typedef struct JxrDecoderSessionPreparation {
    CWMImageStrCodec templateCodec;
    Bool usesHardTileBoundaries;
    Bool isLossyTranscoding;
} JxrDecoderSessionPreparation;

Int JxrDecoderSessionPreparationPrepare(CWMImageInfo* imageInfo,
    CWMIStrCodecParam* codecParameters, JxrDecoderSessionPreparation* preparation);

#endif
