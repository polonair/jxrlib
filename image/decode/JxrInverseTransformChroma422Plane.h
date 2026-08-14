#ifndef JXR_INVERSE_TRANSFORM_CHROMA_422_PLANE_H
#define JXR_INVERSE_TRANSFORM_CHROMA_422_PLANE_H

#include "JxrInverseTransformMacroblockGeometry.h"
#include "JxrInverseTransformPlaneContext.h"

/* Applies both inverse-transform stages for one normal 4:2:2 chroma plane. */
Void JxrInverseTransformChroma422PlaneApply(
    const JxrInverseTransformPlaneContext* plane,
    const JxrInverseTransformMacroblockGeometry* geometry,
    Bool usesScaledArithmetic);

#endif
