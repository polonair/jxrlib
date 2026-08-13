#ifndef JXR_PREDICTION_MATH_H
#define JXR_PREDICTION_MATH_H

#include "strcodec.h"

Int JxrPredictionMathDequantize(Int coefficient, Int quantizationParameter);
Int JxrPredictionMathSaturateAdaptiveCount(Int value);

#endif
