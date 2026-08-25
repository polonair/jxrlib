#ifndef JXR_ENCODER_QUANTIZATION_PIPELINE_H
#define JXR_ENCODER_QUANTIZATION_PIPELINE_H

#include "strcodec.h"

/* Quantize the transformed coefficients of the current macroblock. */
Void JxrEncoderQuantizationPipelineQuantize(CWMImageStrCodec* codec);

#endif
