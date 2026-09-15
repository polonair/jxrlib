#ifndef JXR_TRANSCODE_FRAME_BUFFER_ALLOCATOR_H
#define JXR_TRANSCODE_FRAME_BUFFER_ALLOCATOR_H

#include "windowsmediaphoto.h"
#include "strcodec.h"

typedef struct JxrTranscodeFrameBufferAllocation {
    PixelI* primaryCoefficients;
    PixelI* alphaCoefficients;
    CWMIMBInfo* primaryMacroblocks;
    CWMIMBInfo* alphaMacroblocks;
} JxrTranscodeFrameBufferAllocation;

Int JxrTranscodeFrameBufferAllocatorAllocate(CWMImageStrCodec* encoderCodec,
    const CWMTranscodingParam* parameters, ORIENTATION orientation,
    size_t coefficientUnit, JxrTranscodeFrameBufferAllocation* allocation);

Void JxrTranscodeFrameBufferAllocatorRelease(
    JxrTranscodeFrameBufferAllocation* allocation);

#endif
