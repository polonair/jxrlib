#ifndef JXR_TRANSCODE_ORIENTATION_STATE_H
#define JXR_TRANSCODE_ORIENTATION_STATE_H

#include "windowsmediaphoto.h"

/* Explicit orientation flags; no caller depends on the numeric enum ordering. */
typedef struct JxrTranscodeOrientationState {
    Bool flipVertical;
    Bool flipHorizontal;
    Bool transpose;
} JxrTranscodeOrientationState;

Void JxrTranscodeOrientationStateInit(JxrTranscodeOrientationState* state,
    ORIENTATION orientation);
Int JxrTranscodeOrientationStateMapRow(const JxrTranscodeOrientationState* state,
    Int row, size_t height);
Int JxrTranscodeOrientationStateMapColumn(const JxrTranscodeOrientationState* state,
    Int column, size_t width);
Int JxrTranscodeOrientationStateTileRowCoordinate(const JxrTranscodeOrientationState* state,
    Int row, Int column);
Int JxrTranscodeOrientationStateTileColumnCoordinate(const JxrTranscodeOrientationState* state,
    Int row, Int column);
size_t JxrTranscodeOrientationStateFrameOffset(const JxrTranscodeOrientationState* state,
    Int row, Int column, size_t width, size_t height);

#endif
