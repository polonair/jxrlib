#ifndef JXR_FORWARD_TRANSFORM_CHROMA_422_PLANE_H
#define JXR_FORWARD_TRANSFORM_CHROMA_422_PLANE_H

#include "JxrForwardTransformBoundaryContext.h"

/* Applies both forward-transform stages for one 4:2:2 chroma plane. */
Void JxrForwardTransformChroma422PlaneApply(
    PixelI* firstStage,
    PixelI* secondStage,
    PixelI* predictionBefore,
    PixelI* predictionAfter,
    const JxrForwardTransformMacroblockGeometry* geometry,
    const JxrForwardTransformBoundaryContext* boundaries,
    Bool usesScaledArithmetic);

#endif
