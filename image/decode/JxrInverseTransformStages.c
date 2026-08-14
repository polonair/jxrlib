#include "JxrInverseTransformStages.h"
#include "JxrInverseTransformMath.h"
#include "JxrTransformMath.h"
#include "strTransform.h"

typedef enum JxrInverseTransformStage1OverlapMode { JxrInverseTransformStage1Normal, JxrInverseTransformStage1Alternate } JxrInverseTransformStage1OverlapMode;

Void JxrInverseTransformStagesApplyStage1Idct(PixelI* p)
{
    JxrTransformMathApplyDct2x2Up(p, p + 1, p + 2, p + 3);
    JxrInverseTransformMathApplyOdd(p + 5, p + 4, p + 7, p + 6);
    JxrInverseTransformMathApplyOdd(p + 10, p + 8, p + 11, p + 9);
    JxrInverseTransformMathApplyOddOdd(p + 15, p + 14, p + 13, p + 12);
    JxrTransformMathApplyFourButterfly(p, JxrTransformFirstStageFourButterflyOffsets);
}

static Void JxrInverseTransformStagesApplyStage1Split(PixelI* p0, PixelI* p1, Int offset,
    Int highPassQuantizer, Bool highPassAbsent, JxrInverseTransformStage1OverlapMode mode)
{
    Int column, directCurrent[4], temporaryCurrent;
    PixelI* p2 = p0 + 72 - offset;
    PixelI* p3 = p1 + 64 - offset;
    p0 += 12; p1 += 4;
    for (column = 0; column < 4; ++column) JxrTransformMathApplyDct2x2Down(p0 + column, p2 + column, p1 + column, p3 + column);
    JxrInverseTransformMathApplyOddOddPost(p3, p3 + 1, p3 + 2, p3 + 3);
    JxrInverseTransformMathRotateHalf(&p1[2], &p1[3]); JxrInverseTransformMathRotateHalf(&p1[0], &p1[1]);
    JxrInverseTransformMathRotateHalf(&p2[1], &p2[3]); JxrInverseTransformMathRotateHalf(&p2[0], &p2[2]);
    for (column = 0; column < 4; ++column) {
        if (mode == JxrInverseTransformStage1Alternate) JxrInverseTransformMathApplyAlternateHadamardScale2(p0 + column, p3 + column);
        else JxrInverseTransformMathApplyHadamardScale2(p0 + column, p3 + column);
    }
    for (column = 0; column < 4; ++column) JxrInverseTransformMathApplyHadamardScale4(p0 + column, p2 + column, p1 + column, p3 + column);
    if (mode == JxrInverseTransformStage1Alternate) return;
    for (column = 0; column < 4; ++column) { temporaryCurrent = (p0[column] + p1[column] + p2[column] + p3[column]) >> 1; directCurrent[column] = (temporaryCurrent * 595 + 65536) >> 17; }
    for (column = 0; column < 4; ++column) JxrInverseTransformMathApplyConditionalDcCompensation(p0 + column, p2 + column, p1 + column, p3 + column, directCurrent[column], highPassQuantizer, highPassAbsent);
}

Void JxrInverseTransformStagesApplyStage1SplitNormal(PixelI* first, PixelI* second, Int offset, Int highPassQuantizer, Bool highPassAbsent)
{ JxrInverseTransformStagesApplyStage1Split(first, second, offset, highPassQuantizer, highPassAbsent, JxrInverseTransformStage1Normal); }
Void JxrInverseTransformStagesApplyStage1SplitAlternate(PixelI* first, PixelI* second, Int offset)
{ JxrInverseTransformStagesApplyStage1Split(first, second, offset, 0, FALSE, JxrInverseTransformStage1Alternate); }

static const Int JxrInverseTransformStage2FirstOffsets[4] = { -96, -32, -80, -16 };
static const Int JxrInverseTransformStage2SecondOffsets[4] = { 96, 32, 112, 48 };
static const Int JxrInverseTransformStage2ThirdOffsets[4] = { -112, -48, -128, -64 };
static const Int JxrInverseTransformStage2FourthOffsets[4] = { 80, 16, 64, 0 };

Void JxrInverseTransformStagesApplyStage2Idct(PixelI* p)
{
    JxrInverseTransformMathApplyOdd(p + 32, p + 48, p + 96, p + 112);
    JxrInverseTransformMathApplyOdd(p + 128, p + 192, p + 144, p + 208);
    JxrInverseTransformMathApplyOddOdd(p + 160, p + 224, p + 176, p + 240);
    JxrTransformMathApplyDct2x2Up(p, p + 64, p + 16, p + 80);
    JxrTransformMathApplyFourButterfly(p, JxrTransformSecondStageFourButterflyOffsets);
}

static Void JxrInverseTransformStagesApplyStage2Split(PixelI* first, PixelI* second, Bool alternate)
{
    Int column;
    for (column = 0; column < 4; ++column) JxrTransformMathApplyDct2x2Down(
        first + JxrInverseTransformStage2FirstOffsets[column], first + JxrInverseTransformStage2SecondOffsets[column],
        second + JxrInverseTransformStage2ThirdOffsets[column], second + JxrInverseTransformStage2FourthOffsets[column]);
    JxrInverseTransformMathApplyOddOddPost(second, second + 64, second + 16, second + 80);
    JxrInverseTransformMathRotateHalf(&first[48], &first[32]); JxrInverseTransformMathRotateHalf(&first[112], &first[96]);
    JxrInverseTransformMathRotateHalf(&second[-64], &second[-128]); JxrInverseTransformMathRotateHalf(&second[-48], &second[-112]);
    for (column = 0; column < 4; ++column) {
        if (alternate) JxrInverseTransformMathApplyAlternateHadamardScale2(first + JxrInverseTransformStage2FirstOffsets[column], second + JxrInverseTransformStage2FourthOffsets[column]);
        else JxrInverseTransformMathApplyHadamardScale2(first + JxrInverseTransformStage2FirstOffsets[column], second + JxrInverseTransformStage2FourthOffsets[column]);
    }
    for (column = 0; column < 4; ++column) JxrInverseTransformMathApplyHadamardScale4(
        first + JxrInverseTransformStage2FirstOffsets[column], second + JxrInverseTransformStage2ThirdOffsets[column],
        first + JxrInverseTransformStage2SecondOffsets[column], second + JxrInverseTransformStage2FourthOffsets[column]);
}
Void JxrInverseTransformStagesApplyStage2SplitNormal(PixelI* first, PixelI* second)
{ JxrInverseTransformStagesApplyStage2Split(first, second, FALSE); }
Void JxrInverseTransformStagesApplyStage2SplitAlternate(PixelI* first, PixelI* second)
{ JxrInverseTransformStagesApplyStage2Split(first, second, TRUE); }
