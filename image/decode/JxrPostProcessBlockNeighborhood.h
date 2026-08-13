//*@@@+++@@@@******************************************************************
//
// Copyright (c) Microsoft Corp.
// All rights reserved.
//
//*@@@---@@@@******************************************************************

#ifndef JXR_POST_PROCESS_BLOCK_NEIGHBORHOOD_H
#define JXR_POST_PROCESS_BLOCK_NEIGHBORHOOD_H

#include "strcodec.h"

/*
 * A 5x5 view of the blocks needed when processing macroblock A.  The upper
 * left 4x4 area belongs to A; the final row and column contain the adjacent
 * blocks from C, B and D.
 */
typedef struct JxrPostProcessBlockNeighborhood {
    Int dc[5][5];
    U8 texture[5][5];
} JxrPostProcessBlockNeighborhood;

Void JxrPostProcessBlockNeighborhoodLoad(
    JxrPostProcessBlockNeighborhood* neighborhood,
    const struct tagPostProcInfo* macroblockA,
    const struct tagPostProcInfo* macroblockB,
    const struct tagPostProcInfo* macroblockC,
    const struct tagPostProcInfo* macroblockD);

Bool JxrPostProcessBlockNeighborhoodShouldSmoothHorizontal(
    const JxrPostProcessBlockNeighborhood* neighborhood,
    size_t blockRow,
    size_t blockColumn,
    Int threshold);

Bool JxrPostProcessBlockNeighborhoodShouldSmoothVertical(
    const JxrPostProcessBlockNeighborhood* neighborhood,
    size_t blockRow,
    size_t blockColumn,
    Int threshold);

#endif
