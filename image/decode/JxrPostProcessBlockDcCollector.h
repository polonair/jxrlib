//*@@@+++@@@@******************************************************************
//
// Copyright (c) Microsoft Corp.
// All rights reserved.
//
//*@@@---@@@@******************************************************************

#ifndef JXR_POST_PROCESS_BLOCK_DC_COLLECTOR_H
#define JXR_POST_PROCESS_BLOCK_DC_COLLECTOR_H

#include "strcodec.h"

/*
 * Captures block DC values after macroblock-edge smoothing.  previousSamples
 * and currentSamples point to the start of the adjacent macroblock rows.
 */
Void JxrPostProcessBlockDcCollectorCollect(
    const PixelI* previousSamples,
    const PixelI* currentSamples,
    struct tagPostProcInfo* macroblockA,
    struct tagPostProcInfo* macroblockB,
    struct tagPostProcInfo* macroblockC,
    struct tagPostProcInfo* macroblockD);

#endif
