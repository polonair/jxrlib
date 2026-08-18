#ifndef JXR_FORWARD_TRANSFORM_MACROBLOCK_GEOMETRY_H
#define JXR_FORWARD_TRANSFORM_MACROBLOCK_GEOMETRY_H

#include "strcodec.h"

/* Immutable macroblock facts consumed by the forward transform. */
typedef struct JxrForwardTransformMacroblockGeometry {
    OVERLAP overlap;
    COLORFORMAT colorFormat;
    Bool isLeft;
    Bool isRight;
    Bool isTop;
    Bool isBottom;
    Bool isTopOrBottom;
    Bool isLeftOrRight;
    Bool isTopOrLeft;
    Bool isLeftAdjacentColumn;
    Bool isRightAdjacentColumn;
    size_t fullResolutionPlaneCount;
} JxrForwardTransformMacroblockGeometry;

Void JxrForwardTransformMacroblockGeometryInitialize(
    JxrForwardTransformMacroblockGeometry* geometry,
    OVERLAP overlap,
    COLORFORMAT colorFormat,
    size_t macroblockColumn,
    size_t macroblockRow,
    size_t macroblockWidth,
    size_t macroblockHeight,
    size_t nativeChannelCount);

#endif
