//*@@@+++@@@@******************************************************************
//
// Copyright (c) Microsoft Corp.
// All rights reserved.
//
//*@@@---@@@@******************************************************************

#include "JxrPostProcessBlockNeighborhood.h"

#include "JxrPostProcessDecision.h"

Void JxrPostProcessBlockNeighborhoodLoad(
    JxrPostProcessBlockNeighborhood* neighborhood,
    const struct tagPostProcInfo* macroblockA,
    const struct tagPostProcInfo* macroblockB,
    const struct tagPostProcInfo* macroblockC,
    const struct tagPostProcInfo* macroblockD)
{
    size_t blockRow;
    size_t blockColumn;

    for (blockRow = 0; blockRow < 4; ++blockRow) {
        for (blockColumn = 0; blockColumn < 4; ++blockColumn) {
            neighborhood->dc[blockRow][blockColumn] = macroblockA->iBlockDC[blockRow][blockColumn];
            neighborhood->texture[blockRow][blockColumn] = macroblockA->ucBlockTexture[blockRow][blockColumn];
        }

        neighborhood->dc[4][blockRow] = macroblockC->iBlockDC[0][blockRow];
        neighborhood->texture[4][blockRow] = macroblockC->ucBlockTexture[0][blockRow];
        neighborhood->dc[blockRow][4] = macroblockB->iBlockDC[blockRow][0];
        neighborhood->texture[blockRow][4] = macroblockB->ucBlockTexture[blockRow][0];
    }

    neighborhood->dc[4][4] = macroblockD->iBlockDC[0][0];
    neighborhood->texture[4][4] = macroblockD->ucBlockTexture[0][0];
}

Bool JxrPostProcessBlockNeighborhoodShouldSmoothHorizontal(
    const JxrPostProcessBlockNeighborhood* neighborhood,
    size_t blockRow,
    size_t blockColumn,
    Int threshold)
{
    return JxrPostProcessShouldDeblockBoundary(
        neighborhood->texture[blockRow][blockColumn],
        neighborhood->dc[blockRow][blockColumn],
        neighborhood->texture[blockRow + 1][blockColumn],
        neighborhood->dc[blockRow + 1][blockColumn],
        threshold);
}

Bool JxrPostProcessBlockNeighborhoodShouldSmoothVertical(
    const JxrPostProcessBlockNeighborhood* neighborhood,
    size_t blockRow,
    size_t blockColumn,
    Int threshold)
{
    return JxrPostProcessShouldDeblockBoundary(
        neighborhood->texture[blockRow][blockColumn],
        neighborhood->dc[blockRow][blockColumn],
        neighborhood->texture[blockRow][blockColumn + 1],
        neighborhood->dc[blockRow][blockColumn + 1],
        threshold);
}
