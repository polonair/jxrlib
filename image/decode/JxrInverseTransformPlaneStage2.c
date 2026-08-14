#include "JxrInverseTransformPlaneStage2.h"

#include "JxrInverseTransformMath.h"
#include "strTransform.h"

extern Void strIDCT4x4Stage2(PixelI* samples);

Void JxrInverseTransformPlaneStage2Apply(
    PixelI* secondStage,
    Bool chroma,
    Bool usesScaledArithmetic)
{
    strIDCT4x4Stage2(secondStage);
    if (usesScaledArithmetic) {
        JxrInverseTransformMathNormalizeBlock(secondStage, chroma, 256, 16);
    }
}
