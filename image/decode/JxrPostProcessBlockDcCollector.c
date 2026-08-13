//*@@@+++@@@@******************************************************************
//
// Copyright (c) Microsoft Corp.
// All rights reserved.
//
//*@@@---@@@@******************************************************************

#include "JxrPostProcessBlockDcCollector.h"

typedef struct JxrPostProcessBlockDcPosition {
    Int sampleOffset;
    U8 blockRow;
    U8 blockColumn;
} JxrPostProcessBlockDcPosition;

static const JxrPostProcessBlockDcPosition JxrPostProcessBottomRightBlockPositions[4] = {
    { 0, 0, 0 }, { 64, 0, 1 }, { 16, 1, 0 }, { 80, 1, 1 }
};

static const JxrPostProcessBlockDcPosition JxrPostProcessTopRightBlockPositions[4] = {
    { 32, 2, 0 }, { 96, 2, 1 }, { 48, 3, 0 }, { 112, 3, 1 }
};

static const JxrPostProcessBlockDcPosition JxrPostProcessBottomLeftBlockPositions[4] = {
    { -128, 0, 2 }, { -64, 0, 3 }, { -112, 1, 2 }, { -48, 1, 3 }
};

static const JxrPostProcessBlockDcPosition JxrPostProcessTopLeftBlockPositions[4] = {
    { -96, 2, 2 }, { -32, 2, 3 }, { -80, 3, 2 }, { -16, 3, 3 }
};

static Void JxrPostProcessBlockDcCollectorApply(
    const PixelI* samples,
    struct tagPostProcInfo* macroblock,
    const JxrPostProcessBlockDcPosition positions[4])
{
    size_t positionIndex;

    for (positionIndex = 0; positionIndex < 4; ++positionIndex) {
        const JxrPostProcessBlockDcPosition* position = &positions[positionIndex];
        macroblock->iBlockDC[position->blockRow][position->blockColumn] = samples[position->sampleOffset];
    }
}

Void JxrPostProcessBlockDcCollectorCollect(
    const PixelI* previousSamples,
    const PixelI* currentSamples,
    struct tagPostProcInfo* macroblockA,
    struct tagPostProcInfo* macroblockB,
    struct tagPostProcInfo* macroblockC,
    struct tagPostProcInfo* macroblockD)
{
    JxrPostProcessBlockDcCollectorApply(currentSamples, macroblockD, JxrPostProcessBottomRightBlockPositions);
    JxrPostProcessBlockDcCollectorApply(previousSamples, macroblockB, JxrPostProcessTopRightBlockPositions);
    JxrPostProcessBlockDcCollectorApply(currentSamples, macroblockC, JxrPostProcessBottomLeftBlockPositions);
    JxrPostProcessBlockDcCollectorApply(previousSamples, macroblockA, JxrPostProcessTopLeftBlockPositions);
}
