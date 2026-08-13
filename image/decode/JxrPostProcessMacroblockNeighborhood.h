//*@@@+++@@@@******************************************************************
//
// Copyright (c) Microsoft Corp.
// All rights reserved.
//
//*@@@---@@@@******************************************************************

#ifndef JXR_POST_PROCESS_MACROBLOCK_NEIGHBORHOOD_H
#define JXR_POST_PROCESS_MACROBLOCK_NEIGHBORHOOD_H

#include "strcodec.h"

/* Four macroblocks around the currently processed lower-right macroblock. */
typedef struct JxrPostProcessMacroblockNeighborhood {
    struct tagPostProcInfo* topLeft;
    struct tagPostProcInfo* topRight;
    struct tagPostProcInfo* bottomLeft;
    struct tagPostProcInfo* bottomRight;
} JxrPostProcessMacroblockNeighborhood;

Void JxrPostProcessMacroblockNeighborhoodLoad(
    JxrPostProcessMacroblockNeighborhood* neighborhood,
    struct tagPostProcInfo* rows[MAX_CHANNELS][2],
    size_t channel,
    size_t macroblockX);

#endif
