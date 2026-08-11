#ifndef JXR_TRANSCODE_COEFFICIENT_TRANSFORM_H
#define JXR_TRANSCODE_COEFFICIENT_TRANSFORM_H

#include "JxrTranscodeOrientationState.h"

typedef struct JxrTranscodeCoefficientBuffer {
    PixelI* values;
    size_t offset;
    size_t count;
} JxrTranscodeCoefficientBuffer;

Void JxrTranscodeCoefficientBufferInit(JxrTranscodeCoefficientBuffer* buffer,
    PixelI* values, size_t offset, size_t count);
Bool JxrTranscodeCoefficientTransformDc444(JxrTranscodeCoefficientBuffer* source,
    JxrTranscodeCoefficientBuffer* destination,
    const JxrTranscodeOrientationState* orientation);
Bool JxrTranscodeCoefficientTransformAc444(JxrTranscodeCoefficientBuffer* source,
    JxrTranscodeCoefficientBuffer* destination,
    const JxrTranscodeOrientationState* orientation);
Bool JxrTranscodeCoefficientTransformDc422(JxrTranscodeCoefficientBuffer* source,
    JxrTranscodeCoefficientBuffer* destination,
    const JxrTranscodeOrientationState* orientation);
Bool JxrTranscodeCoefficientTransformAc422(JxrTranscodeCoefficientBuffer* source,
    JxrTranscodeCoefficientBuffer* destination,
    const JxrTranscodeOrientationState* orientation);
Bool JxrTranscodeCoefficientTransformDc420(JxrTranscodeCoefficientBuffer* source,
    JxrTranscodeCoefficientBuffer* destination,
    const JxrTranscodeOrientationState* orientation);
Bool JxrTranscodeCoefficientTransformAc420(JxrTranscodeCoefficientBuffer* source,
    JxrTranscodeCoefficientBuffer* destination,
    const JxrTranscodeOrientationState* orientation);

#endif
