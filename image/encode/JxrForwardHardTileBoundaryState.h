#ifndef JXR_FORWARD_HARD_TILE_BOUNDARY_STATE_H
#define JXR_FORWARD_HARD_TILE_BOUNDARY_STATE_H

#include "windowsmediaphoto.h"

typedef struct JxrForwardHardTileBoundaryConfiguration {
    Bool enabled;
    U32 verticalSliceCountMinusOne;
    U32 horizontalSliceCountMinusOne;
    const U32* verticalSliceColumns;
    const U32* horizontalSliceRows;
} JxrForwardHardTileBoundaryConfiguration;

typedef struct JxrForwardHardTileBoundaryState {
    size_t tileX;
    size_t tileY;
    size_t previousMacroblockX;
    size_t previousMacroblockY;
    Bool isVerticalBoundary;
    Bool isHorizontalBoundary;
    Bool isOneMacroblockLeftOfVerticalBoundary;
    Bool isOneMacroblockRightOfVerticalBoundary;
} JxrForwardHardTileBoundaryState;

Void JxrForwardHardTileBoundaryStateCalculate(
    JxrForwardHardTileBoundaryState* result,
    const JxrForwardHardTileBoundaryState* previous,
    const JxrForwardHardTileBoundaryConfiguration* configuration,
    size_t macroblockColumn,
    size_t macroblockRow);

#endif
