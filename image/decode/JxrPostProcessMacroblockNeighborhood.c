//*@@@+++@@@@******************************************************************
//
// Copyright (c) Microsoft Corp.
// All rights reserved.
//
//*@@@---@@@@******************************************************************

#include "JxrPostProcessMacroblockNeighborhood.h"

Void JxrPostProcessMacroblockNeighborhoodLoad(
    JxrPostProcessMacroblockNeighborhood* neighborhood,
    struct tagPostProcInfo* rows[MAX_CHANNELS][2],
    size_t channel,
    size_t macroblockX)
{
    neighborhood->topRight = rows[channel][0] + macroblockX;
    neighborhood->topLeft = neighborhood->topRight - 1;
    neighborhood->bottomRight = rows[channel][1] + macroblockX;
    neighborhood->bottomLeft = neighborhood->bottomRight - 1;
}
