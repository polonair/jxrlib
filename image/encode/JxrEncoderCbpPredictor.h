#ifndef JXR_ENCODER_CBP_PREDICTOR_H
#define JXR_ENCODER_CBP_PREDICTOR_H

#include "strcodec.h"

/* Clamps an adaptive CBP model counter to the JPEG XR range [-16, 15]. */
Int JxrEncoderCbpPredictorClampModelCount(Int value);

/* Updates one adaptive CBP model after the number of coded coefficients is known. */
Void JxrEncoderCbpPredictorUpdateModel(CCBPModel* model, size_t modelIndex,
    Int originalCoefficientCount);

/* Computes and predicts the coded-block pattern for the current macroblock. */
Void JxrEncoderCbpPredictorApply(CWMImageStrCodec* codec,
    CCodingContext* codingContext);

#endif
