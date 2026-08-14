#ifndef JXR_INVERSE_TRANSFORM_MACROBLOCK_GEOMETRY_H
#define JXR_INVERSE_TRANSFORM_MACROBLOCK_GEOMETRY_H

#include "strcodec.h"

/* Immutable macroblock facts consumed by the inverse transform. */
typedef struct JxrInverseTransformMacroblockGeometry {
    OVERLAP overlap;
    COLORFORMAT colorFormat;
    Bool isLeft;
    Bool isRight;
    Bool isTop;
    Bool isBottom;
    Bool isTopOrBottom;
    Bool isLeftOrRight;
    Bool isTopOrLeft;
    Bool isBottomOrRight;
    Bool isLeftAdjacentColumn;
    Bool isRightAdjacentColumn;
    size_t macroblockWidth;
    size_t macroblockColumn;
    size_t channelCount;
    size_t thumbnailScale;
} JxrInverseTransformMacroblockGeometry;

Void JxrInverseTransformMacroblockGeometryInitialize(
    JxrInverseTransformMacroblockGeometry* geometry,
    OVERLAP overlap,
    COLORFORMAT colorFormat,
    size_t macroblockColumn,
    size_t macroblockRow,
    size_t macroblockWidth,
    size_t macroblockHeight,
    size_t nativeChannelCount,
    size_t thumbnailScale);

#endif
