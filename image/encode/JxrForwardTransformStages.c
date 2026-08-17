#include "strTransform.h"
#include "JxrTransformMath.h"
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

Void JxrForwardTransformStagesApplyPreStage1Split(
    PixelI* firstStage,
    PixelI* secondStage,
    Int offset)
{
    PixelI* firstBottom = firstStage + 12;
    PixelI* secondBottom = secondStage + 4;
    PixelI* firstTop = firstStage + 72 - offset;
    PixelI* secondTop = secondStage + 64 - offset;
    Int column;

    for (column = 0; column < 4; ++column) {
        JxrForwardTransformMathApplyHst4(firstBottom + column, firstTop + column,
            secondBottom + column, secondTop + column);
    }
    for (column = 0; column < 4; ++column) {
        JxrForwardTransformMathApplyHst1(firstBottom + column, secondTop + column);
    }
    JxrForwardTransformMathRotateHalf(&secondBottom[2], &secondBottom[3]);
    JxrForwardTransformMathRotateHalf(&secondBottom[0], &secondBottom[1]);
    JxrForwardTransformMathRotateHalf(&firstTop[1], &firstTop[3]);
    JxrForwardTransformMathRotateHalf(&firstTop[0], &firstTop[2]);
    JxrForwardTransformMathApplyOddOddPre(secondTop, secondTop + 1,
        secondTop + 2, secondTop + 3);
    for (column = 0; column < 4; ++column) {
        JxrTransformMathApplyDct2x2Down(firstBottom + column, firstTop + column,
            secondBottom + column, secondTop + column);
    }
}
