#ifndef JXR_FORWARD_TRANSFORM_CHROMA_420_PLANE_H
#define JXR_FORWARD_TRANSFORM_CHROMA_420_PLANE_H

#include "JxrForwardTransformBoundaryContext.h"

/* Applies both forward-transform stages for one 4:2:0 chroma plane. */
Void JxrForwardTransformChroma420PlaneApply(
    PixelI* firstStage,
    PixelI* secondStage,
    PixelI* predictionBefore,
    PixelI* predictionAfter,
    const JxrForwardTransformMacroblockGeometry* geometry,
    const JxrForwardTransformBoundaryContext* boundaries,
    Bool usesScaledArithmetic);

#endif
