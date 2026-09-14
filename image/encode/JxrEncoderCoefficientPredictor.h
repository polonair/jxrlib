#ifndef JXR_ENCODER_COEFFICIENT_PREDICTOR_H
#define JXR_ENCODER_COEFFICIENT_PREDICTOR_H

#include "strcodec.h"

/* Applies forward frequency-domain DC, AD and AC prediction. */
Void JxrEncoderCoefficientPredictorApply(CWMImageStrCodec* codec);

#endif
