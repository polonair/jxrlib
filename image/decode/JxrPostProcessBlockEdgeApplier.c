//*@@@+++@@@@******************************************************************
//
// Copyright (c) Microsoft Corp.
// All rights reserved.
//
//*@@@---@@@@******************************************************************

#include "JxrPostProcessBlockEdgeApplier.h"

#include "strcodec.h"
#include "JxrPostProcessSmoothing.h"

static PixelI* JxrPostProcessBlockEdgeApplierGetBlockStart(
    PixelI* previousMacroblockRow,
    size_t blockRow,
    size_t blockColumn)
{
    return previousMacroblockRow - 256 + blockColumn * 64 + blockRow * 16;
}

Void JxrPostProcessBlockEdgeApplierApplyHorizontal(
    PixelI* previousMacroblockRow,
    PixelI* currentMacroblockRow,
    size_t blockRow,
    size_t blockColumn)
{
    PixelI* upperBlock = JxrPostProcessBlockEdgeApplierGetBlockStart(
        previousMacroblockRow, blockRow, blockColumn);
    PixelI* lowerBlock = blockRow < 3 ? upperBlock + 16 :
        currentMacroblockRow - 256 + blockColumn * 64;
    size_t sampleLine;

    for (sampleLine = 0; sampleLine < 4; ++sampleLine) {
        JxrPostProcessSmoothingApplyBlockEdge(
            upperBlock + idxCC[1][sampleLine],
            upperBlock + idxCC[2][sampleLine],
            upperBlock + idxCC[3][sampleLine],
            lowerBlock + idxCC[0][sampleLine],
            lowerBlock + idxCC[1][sampleLine],
            lowerBlock + idxCC[2][sampleLine]);
    }
}

Void JxrPostProcessBlockEdgeApplierApplyVertical(
    PixelI* previousMacroblockRow,
    size_t blockRow,
    size_t blockColumn)
{
    PixelI* leftBlock = JxrPostProcessBlockEdgeApplierGetBlockStart(
        previousMacroblockRow, blockRow, blockColumn);
    PixelI* rightBlock = leftBlock + 64;
    size_t sampleLine;

    for (sampleLine = 0; sampleLine < 4; ++sampleLine) {
        JxrPostProcessSmoothingApplyBlockEdge(
            leftBlock + idxCC[sampleLine][1],
            leftBlock + idxCC[sampleLine][2],
            leftBlock + idxCC[sampleLine][3],
            rightBlock + idxCC[sampleLine][0],
            rightBlock + idxCC[sampleLine][1],
            rightBlock + idxCC[sampleLine][2]);
    }
}
