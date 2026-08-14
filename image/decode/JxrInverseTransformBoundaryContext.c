#include "JxrInverseTransformBoundaryContext.h"

Void JxrInverseTransformBoundaryContextInitialize(
    JxrInverseTransformBoundaryContext* context,
    const JxrInverseTransformMacroblockGeometry* geometry,
    const JxrHardTileBoundaryState* hardTileState)
{
    context->isVerticalTileBoundary = hardTileState->isVerticalBoundary;
    context->isHorizontalTileBoundary = hardTileState->isHorizontalBoundary;
    context->hasTopBoundary = geometry->isTop || hardTileState->isHorizontalBoundary;
    context->hasBottomBoundary = geometry->isBottom || hardTileState->isHorizontalBoundary;
    context->hasLeftBoundary = geometry->isLeft || hardTileState->isVerticalBoundary;
    context->hasRightBoundary = geometry->isRight || hardTileState->isVerticalBoundary;
    context->hasTopOrBottomBoundary = geometry->isTopOrBottom ||
        hardTileState->isHorizontalBoundary;
    context->hasLeftOrRightBoundary = geometry->isLeftOrRight ||
        hardTileState->isVerticalBoundary;
    context->isLeftAdjacentToVerticalBoundary = geometry->isLeftAdjacentColumn ||
        hardTileState->isOneMacroblockRightOfVerticalBoundary;
    context->isRightAdjacentToVerticalBoundary = geometry->isRightAdjacentColumn ||
        hardTileState->isOneMacroblockLeftOfVerticalBoundary;
}
