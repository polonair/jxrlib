#ifndef JXR_INVERSE_TRANSFORM_CHROMA_420_ALTERNATE_PLANE_H
#define JXR_INVERSE_TRANSFORM_CHROMA_420_ALTERNATE_PLANE_H

#include "JxrInverseTransformBoundaryContext.h"
#include "JxrInverseTransformPlaneContext.h"

/* Applies both inverse-transform stages for one hard-tile 4:2:0 chroma plane. */
Void JxrInverseTransformChroma420AlternatePlaneApply(
    const JxrInverseTransformPlaneContext* plane,
    const JxrInverseTransformMacroblockGeometry* geometry,
    const JxrInverseTransformBoundaryContext* boundaries,
    Bool usesScaledArithmetic,
    PixelI* predictionBefore,
    PixelI* predictionAfter);

#endif
