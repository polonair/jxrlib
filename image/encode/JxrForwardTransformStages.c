#include "strTransform.h"
#include "JxrForwardTransformMath.h"
#include "JxrForwardTransformStages.h"

Void JxrForwardTransformStagesApplyStage1Dct(PixelI* samples)
{
    strTransformApplyFourButterfly(samples, JxrTransformFirstStageFourButterflyOffsets);
    strDCT2x2up(&samples[0], &samples[1], &samples[2], &samples[3]);
    JxrForwardTransformMathApplyOddOdd(&samples[15], &samples[14], &samples[13], &samples[12]);
    JxrForwardTransformMathApplyOdd(&samples[5], &samples[4], &samples[7], &samples[6]);
    JxrForwardTransformMathApplyOdd(&samples[10], &samples[8], &samples[11], &samples[9]);
}

Void JxrForwardTransformStagesApplyStage2Dct(PixelI* samples)
{
    strTransformApplyFourButterfly(samples, JxrTransformSecondStageFourButterflyOffsets);
    strDCT2x2up(&samples[0], &samples[64], &samples[16], &samples[80]);
    JxrForwardTransformMathApplyOddOdd(&samples[160], &samples[224], &samples[176], &samples[240]);
    JxrForwardTransformMathApplyOdd(&samples[128], &samples[192], &samples[144], &samples[208]);
    JxrForwardTransformMathApplyOdd(&samples[32], &samples[48], &samples[96], &samples[112]);
}
