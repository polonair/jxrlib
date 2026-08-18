#include "JxrForwardTransformMacroblockGeometry.h"

Void JxrForwardTransformMacroblockGeometryInitialize(
    JxrForwardTransformMacroblockGeometry* geometry,
    OVERLAP overlap,
    COLORFORMAT colorFormat,
    size_t macroblockColumn,
    size_t macroblockRow,
    size_t macroblockWidth,
    size_t macroblockHeight,
    size_t nativeChannelCount)
{
    geometry->overlap = overlap;
    geometry->colorFormat = colorFormat;
    geometry->isLeft = macroblockColumn == 0;
    geometry->isRight = macroblockColumn == macroblockWidth;
    geometry->isTop = macroblockRow == 0;
    geometry->isBottom = macroblockRow == macroblockHeight;
    geometry->isTopOrBottom = geometry->isTop || geometry->isBottom;
    geometry->isLeftOrRight = geometry->isLeft || geometry->isRight;
    geometry->isTopOrLeft = geometry->isTop || geometry->isLeft;
    geometry->isLeftAdjacentColumn = macroblockColumn == 1;
    geometry->isRightAdjacentColumn = macroblockColumn == macroblockWidth - 1;
    geometry->fullResolutionPlaneCount = (colorFormat == YUV_420 || colorFormat == YUV_422) ?
        1 : nativeChannelCount;
}
