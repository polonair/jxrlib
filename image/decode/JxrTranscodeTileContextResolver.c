#include "JxrTranscodeTileContextResolver.h"

Bool JxrTranscodeTileContextResolverResolve(
    const JxrTranscodeTileContextRequest* request,
    JxrTranscodeTileContextResult* result)
{
    size_t index;
    Int tileRowCoordinate;
    Int tileColumnCoordinate;

    if (request == NULL || result == NULL || request->orientation == NULL ||
        request->tileColumns == NULL || request->tileRows == NULL ||
        request->tileColumnCount == 0 || request->tileRowCount == 0 ||
        request->macroblockLeft > request->macroblockRight ||
        request->macroblockTop > request->macroblockBottom ||
        request->macroblockWidth == 0 || request->macroblockHeight == 0) return FALSE;
    memset(result, 0, sizeof(*result));
    if (request->sourceRow < request->macroblockTop ||
        request->sourceRow >= request->macroblockBottom ||
        request->sourceColumn < request->macroblockLeft ||
        request->sourceColumn >= request->macroblockRight) return TRUE;

    result->isInsideRoi = TRUE;
    result->destinationRow = JxrTranscodeOrientationStateMapRow(request->orientation,
        (Int)(request->sourceRow - request->macroblockTop), request->macroblockHeight);
    result->destinationColumn = JxrTranscodeOrientationStateMapColumn(request->orientation,
        (Int)(request->sourceColumn - request->macroblockLeft), request->macroblockWidth);
    tileRowCoordinate = JxrTranscodeOrientationStateTileRowCoordinate(request->orientation,
        result->destinationRow, result->destinationColumn);
    tileColumnCoordinate = JxrTranscodeOrientationStateTileColumnCoordinate(request->orientation,
        result->destinationRow, result->destinationColumn);
    for (index = 0; index < request->tileRowCount; ++index)
        if (request->tileRows[index] == (U32)tileRowCoordinate) {
            result->tileRow = index;
            result->isTileRowStart = TRUE;
            break;
        }
    for (index = 0; index < request->tileColumnCount; ++index)
        if (request->tileColumns[index] == (U32)tileColumnCoordinate) {
            result->tileColumn = index;
            result->isTileColumnStart = TRUE;
            break;
        }
    return TRUE;
}
