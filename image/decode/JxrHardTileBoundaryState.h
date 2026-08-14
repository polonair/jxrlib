#ifndef JXR_HARD_TILE_BOUNDARY_STATE_H
#define JXR_HARD_TILE_BOUNDARY_STATE_H

#include "strcodec.h"

/* Immutable hard-tile boundary inputs read from the image header. */
typedef struct JxrHardTileBoundaryConfiguration {
    Bool enabled;
    U32 verticalSliceCountMinusOne;
    U32 horizontalSliceCountMinusOne;
    const U32* verticalSliceColumns;
    const U32* horizontalSliceRows;
} JxrHardTileBoundaryConfiguration;

/* Mutable cursor and boundary decisions for one decoded macroblock. */
typedef struct JxrHardTileBoundaryState {
    size_t tileX;
    size_t tileY;
    size_t previousMacroblockX;
    size_t previousMacroblockY;
    Bool isVerticalBoundary;
    Bool isHorizontalBoundary;
    Bool isOneMacroblockLeftOfVerticalBoundary;
    Bool isOneMacroblockRightOfVerticalBoundary;
} JxrHardTileBoundaryState;

Void JxrHardTileBoundaryStateCalculate(
    JxrHardTileBoundaryState* result,
    const JxrHardTileBoundaryState* previous,
    const JxrHardTileBoundaryConfiguration* configuration,
    size_t macroblockColumn,
    size_t macroblockRow);

#endif
