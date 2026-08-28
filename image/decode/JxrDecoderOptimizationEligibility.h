#ifndef JXR_DECODER_OPTIMIZATION_ELIGIBILITY_H
#define JXR_DECODER_OPTIMIZATION_ELIGIBILITY_H

#include "strcodec.h"

/* Native-only predicates; managed ports always use the portable path. */
Bool JxrDecoderOptimizationEligibilityCanUseRgb24Output(
    const CWMImageStrCodec* codec);
Bool JxrDecoderOptimizationEligibilityCanUseYuv444CenterTransform(
    const CWMImageStrCodec* codec);

#endif
