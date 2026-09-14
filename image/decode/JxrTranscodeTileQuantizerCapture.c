#include "JxrTranscodeTileQuantizerCapture.h"

Bool JxrTranscodeTileQuantizerCaptureSelectIndex(size_t stateCount,
    size_t tileRow, size_t tileColumn, size_t tileColumnCount,
    Bool storeByDestinationTile, size_t* stateIndex)
{
    size_t index;

    if (stateIndex == NULL || stateCount == 0) return FALSE;
    if (!storeByDestinationTile) {
        *stateIndex = 0;
        return TRUE;
    }
    if (tileColumnCount == 0 || tileColumn >= tileColumnCount ||
        tileRow > ((size_t)-1 - tileColumn) / tileColumnCount)
        return FALSE;
    index = tileRow * tileColumnCount + tileColumn;
    if (index >= stateCount) return FALSE;
    *stateIndex = index;
    return TRUE;
}

Bool JxrTranscodeTileQuantizerCaptureCapture(
    const JxrTranscodeTileQuantizerCaptureRequest* request)
{
    size_t stateIndex;
    JxrTranscodeTileQuantizerState* state;

    if (request == NULL || request->states == NULL || request->primaryTile == NULL ||
        request->primaryChannelCount > MAX_CHANNELS ||
        (request->hasAlpha && (request->alphaTile == NULL ||
            request->alphaChannelIndex >= MAX_CHANNELS)) ||
        !JxrTranscodeTileQuantizerCaptureSelectIndex(request->stateCount,
            request->destinationTileRow, request->destinationTileColumn,
            request->destinationTileColumnCount, request->storeByDestinationTile,
            &stateIndex)) return FALSE;
    state = request->states + stateIndex;
    JxrTranscodeTileQuantizerStateInit(state);
    JxrTranscodeTileQuantizerStateCapturePrimary(state, request->primaryTile,
        request->primaryChannelCount, request->subband);
    if (request->hasAlpha)
        JxrTranscodeTileQuantizerStateCaptureAlpha(state, request->alphaTile,
            request->alphaChannelIndex, request->subband);
    return TRUE;
}
