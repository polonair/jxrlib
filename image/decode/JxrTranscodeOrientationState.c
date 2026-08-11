#include "JxrTranscodeOrientationState.h"

Void JxrTranscodeOrientationStateInit(JxrTranscodeOrientationState* state,
    ORIENTATION orientation)
{
    state->flipVertical = FALSE;
    state->flipHorizontal = FALSE;
    state->transpose = FALSE;
    switch (orientation) {
    case O_FLIPV:
        state->flipVertical = TRUE;
        break;
    case O_FLIPH:
        state->flipHorizontal = TRUE;
        break;
    case O_FLIPVH:
        state->flipVertical = TRUE;
        state->flipHorizontal = TRUE;
        break;
    case O_RCW:
        state->flipVertical = TRUE;
        state->transpose = TRUE;
        break;
    case O_RCW_FLIPV:
        state->flipVertical = TRUE;
        state->flipHorizontal = TRUE;
        state->transpose = TRUE;
        break;
    case O_RCW_FLIPH:
        state->transpose = TRUE;
        break;
    case O_RCW_FLIPVH:
        state->flipHorizontal = TRUE;
        state->transpose = TRUE;
        break;
    case O_NONE:
    default:
        break;
    }
}

Int JxrTranscodeOrientationStateMapRow(const JxrTranscodeOrientationState* state,
    Int row, size_t height)
{
    return state->flipVertical ? (Int)height - row - 1 : row;
}

Int JxrTranscodeOrientationStateMapColumn(const JxrTranscodeOrientationState* state,
    Int column, size_t width)
{
    return state->flipHorizontal ? (Int)width - column - 1 : column;
}

Int JxrTranscodeOrientationStateTileRowCoordinate(const JxrTranscodeOrientationState* state,
    Int row, Int column)
{
    return state->transpose ? column : row;
}

Int JxrTranscodeOrientationStateTileColumnCoordinate(const JxrTranscodeOrientationState* state,
    Int row, Int column)
{
    return state->transpose ? row : column;
}

size_t JxrTranscodeOrientationStateFrameOffset(const JxrTranscodeOrientationState* state,
    Int row, Int column, size_t width, size_t height)
{
    return state->transpose ? (size_t)row + height * (size_t)column :
        (size_t)row * width + (size_t)column;
}
