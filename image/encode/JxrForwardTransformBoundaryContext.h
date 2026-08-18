#ifndef JXR_FORWARD_TRANSFORM_BOUNDARY_CONTEXT_H
#define JXR_FORWARD_TRANSFORM_BOUNDARY_CONTEXT_H

#include "JxrForwardHardTileBoundaryState.h"
#include "JxrForwardTransformMacroblockGeometry.h"

/* Read-only boundary decisions consumed by forward overlap operations. */
typedef struct JxrForwardTransformBoundaryContext {
    Bool isVerticalTileBoundary;
    Bool isHorizontalTileBoundary;
    Bool hasTopBoundary;
    Bool hasBottomBoundary;
    Bool hasLeftBoundary;
    Bool hasRightBoundary;
    Bool hasTopOrBottomBoundary;
    Bool hasLeftOrRightBoundary;
    Bool isLeftAdjacentToVerticalBoundary;
    Bool isRightAdjacentToVerticalBoundary;
} JxrForwardTransformBoundaryContext;

Void JxrForwardTransformBoundaryContextInitialize(
    JxrForwardTransformBoundaryContext* context,
    const JxrForwardTransformMacroblockGeometry* geometry,
    const JxrForwardHardTileBoundaryState* hardTileState);

#endif
