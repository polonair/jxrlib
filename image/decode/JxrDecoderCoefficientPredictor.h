#ifndef JXR_DECODER_COEFFICIENT_PREDICTOR_H
#define JXR_DECODER_COEFFICIENT_PREDICTOR_H

#include "strcodec.h"

/* Applies frequency-domain DC/LP prediction and records the AC orientation. */
Void JxrDecoderCoefficientPredictorApplyDcAc(CWMImageStrCodec* codec);

/* Applies frequency-domain HP prediction using the recorded AC orientation. */
Void JxrDecoderCoefficientPredictorApplyAc(CWMImageStrCodec* codec);

#endif
