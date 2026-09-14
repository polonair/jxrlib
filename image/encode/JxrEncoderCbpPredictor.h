#ifndef JXR_ENCODER_CBP_PREDICTOR_H
#define JXR_ENCODER_CBP_PREDICTOR_H

#include "strcodec.h"

/* Clamps an adaptive CBP model counter to the JPEG XR range [-16, 15]. */
Int JxrEncoderCbpPredictorClampModelCount(Int value);

/* Updates one adaptive CBP model after the number of coded coefficients is known. */
Void JxrEncoderCbpPredictorUpdateModel(CCBPModel* model, size_t modelIndex,
    Int originalCoefficientCount);

/* Contract: blockCount is 4/8/16, flcBits is 0..15; caller supplies valid
   indices for all 16 samples of each block. DC (index 0) is ignored.
   In C#: unchecked((uint)sample + threshold) preserves modulo-2^32 arithmetic. */
Int JxrEncoderCbpPredictorCalculate(const PixelI* coefficients, Int baseIndex,
    const Int* blockOffsets, Int blockCount, Int flcBits);

/* cbp and neighbours are nonnegative blockCount-bit masks; modelIndex is 0/1.
   Computes differential using the OLD model state, then updates its counters.
   All mask shifts are within 1..16 bits; no signed negative shift is needed. */
Int JxrEncoderCbpPredictorPredict(Int cbp, Int blockCount,
    Bool isLeftBoundary, Bool isTopBoundary, Int leftCbp, Int topCbp,
    CCBPModel* model, Int modelIndex);

/* Legacy codec adapter. */
Void JxrEncoderCbpPredictorApply(CWMImageStrCodec* codec,
    CCodingContext* codingContext);

#endif
