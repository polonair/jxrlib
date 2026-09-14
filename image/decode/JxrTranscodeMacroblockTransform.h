#ifndef JXR_TRANSCODE_MACROBLOCK_TRANSFORM_H
#define JXR_TRANSCODE_MACROBLOCK_TRANSFORM_H

#include "strcodec.h"
#include "JxrTranscodeOrientationState.h"

/*
 * Explicit bridge between an oriented, buffered source macroblock and the
 * encoder's current macroblock.  Buffer positions are expressed as element
 * offsets so this maps directly to managed array indexing.
 */
typedef struct JxrTranscodeMacroblockTransformState {
    const CWMIMBInfo* sourceMacroblocks;
    PixelI* sourceCoefficients;
    size_t coefficientUnit;
    size_t macroblockOffset;
    CWMImageStrCodec* destinationCodec;
    PixelI* destinationCoefficients;
    const JxrTranscodeOrientationState* orientation;
} JxrTranscodeMacroblockTransformState;

Bool JxrTranscodeMacroblockTransformPrimary(
    const JxrTranscodeMacroblockTransformState* state);
Bool JxrTranscodeMacroblockTransformAlpha(
    const JxrTranscodeMacroblockTransformState* state);

#endif
