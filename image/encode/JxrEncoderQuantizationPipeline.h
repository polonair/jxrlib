#ifndef JXR_ENCODER_QUANTIZATION_PIPELINE_H
#define JXR_ENCODER_QUANTIZATION_PIPELINE_H

#include "strcodec.h"

/* Explicit coefficient kernel shared by the pipeline and conformance tests. */
I32 JxrEncoderQuantizationPipelineQuantizeCoefficient(PixelI value,
    const CWMIQuantizer* quantizer);

/* Quantize the transformed coefficients of the current macroblock. */
Void JxrEncoderQuantizationPipelineQuantize(CWMImageStrCodec* codec);

#endif
