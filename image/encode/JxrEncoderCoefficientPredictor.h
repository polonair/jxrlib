#ifndef JXR_ENCODER_COEFFICIENT_PREDICTOR_H
#define JXR_ENCODER_COEFFICIENT_PREDICTOR_H

#include "strcodec.h"

typedef struct JxrEncoderCoefficientPredictionReferences {
    Int leftDc;
    Int topDc;
    const PixelI* leftAd;
    const PixelI* topAd;
} JxrEncoderCoefficientPredictionReferences;

/* Each buffer is an explicit macroblock coefficient array: 256 entries for
   full resolution, 64 for 4:2:0 chroma, and 128 for 4:2:2 chroma.
   In C#, map PixelI* to int[] and preserve signed arithmetic in unchecked code. */
Void JxrEncoderCoefficientPredictorApplyFullResolution(PixelI* dcCoefficients,
    PixelI* macroblockCoefficients, Int dcMode, Int adMode, Int acMode,
    const JxrEncoderCoefficientPredictionReferences* references);
Void JxrEncoderCoefficientPredictorApplyChroma420(PixelI* dcCoefficients,
    PixelI* macroblockCoefficients, Int dcMode, Int adMode, Int acMode,
    const JxrEncoderCoefficientPredictionReferences* references);
Void JxrEncoderCoefficientPredictorApplyChroma422(PixelI* dcCoefficients,
    PixelI* macroblockCoefficients, Int dcMode, Int adMode, Int acMode,
    const JxrEncoderCoefficientPredictionReferences* references);

/* Legacy adapter that resolves codec state and records the AC orientation. */
Void JxrEncoderCoefficientPredictorApply(CWMImageStrCodec* codec);

#endif
