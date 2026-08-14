#ifndef JXR_INVERSE_TRANSFORM_BOUNDARY_CONTEXT_H
#define JXR_INVERSE_TRANSFORM_BOUNDARY_CONTEXT_H

#include "JxrHardTileBoundaryState.h"
#include "JxrInverseTransformMacroblockGeometry.h"

/* Read-only boundary decisions consumed by inverse overlap operations. */
typedef struct JxrInverseTransformBoundaryContext {
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
} JxrInverseTransformBoundaryContext;

Void JxrInverseTransformBoundaryContextInitialize(
    JxrInverseTransformBoundaryContext* context,
    const JxrInverseTransformMacroblockGeometry* geometry,
    const JxrHardTileBoundaryState* hardTileState);

#endif
