#ifndef JXR_TRANSCODE_DECODER_RUNTIME_INITIALIZER_H
#define JXR_TRANSCODE_DECODER_RUNTIME_INITIALIZER_H

#include "windowsmediaphoto.h"
#include "strcodec.h"

typedef struct JxrTranscodeDecoderRuntimeState {
    PixelI* macroblockBuffer;
    U8* ioHeaderAllocation;
} JxrTranscodeDecoderRuntimeState;

Int JxrTranscodeDecoderRuntimeInitializerAllocateMacroblockBuffer(
    CWMImageStrCodec* decoderCodec, size_t coefficientUnit,
    JxrTranscodeDecoderRuntimeState* state);

Int JxrTranscodeDecoderRuntimeInitializerInitializePrimaryInput(
    CWMImageStrCodec* decoderCodec, JxrTranscodeDecoderRuntimeState* state);

Void JxrTranscodeDecoderRuntimeInitializerRelease(
    JxrTranscodeDecoderRuntimeState* state);

#endif
