#include "JxrInverseTransformMacroblockGeometry.h"

Void JxrInverseTransformMacroblockGeometryInitialize(
    JxrInverseTransformMacroblockGeometry* geometry,
    OVERLAP overlap,
    COLORFORMAT colorFormat,
    size_t macroblockColumn,
    size_t macroblockRow,
    size_t macroblockWidth,
    size_t macroblockHeight,
    size_t nativeChannelCount,
    size_t thumbnailScale)
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
    geometry->isBottomOrRight = geometry->isBottom || geometry->isRight;
    geometry->isLeftAdjacentColumn = macroblockColumn == 1;
    geometry->isRightAdjacentColumn = macroblockColumn == macroblockWidth - 1;
    geometry->macroblockWidth = macroblockWidth;
    geometry->macroblockColumn = macroblockColumn;
    geometry->channelCount = (colorFormat == YUV_420 || colorFormat == YUV_422) ?
        1 : nativeChannelCount;
    geometry->thumbnailScale = thumbnailScale;
}
