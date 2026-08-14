#ifndef JXR_INVERSE_TRANSFORM_CHROMA_420_PLANE_H
#define JXR_INVERSE_TRANSFORM_CHROMA_420_PLANE_H

#include "JxrInverseTransformMacroblockGeometry.h"
#include "JxrInverseTransformPlaneContext.h"

/* Applies both inverse-transform stages for one normal 4:2:0 chroma plane. */
Void JxrInverseTransformChroma420PlaneApply(
    const JxrInverseTransformPlaneContext* plane,
    const JxrInverseTransformMacroblockGeometry* geometry,
    Bool usesScaledArithmetic);

#endif
