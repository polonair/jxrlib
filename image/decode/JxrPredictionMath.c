#include "JxrPredictionMath.h"

Int JxrPredictionMathDequantize(Int coefficient, Int quantizationParameter)
{
    return coefficient * quantizationParameter;
}

Int JxrPredictionMathSaturateAdaptiveCount(Int value)
{
    if ((U32)(value + 16) >= 32) return value < 0 ? -16 : 15;
    return value;
}
