//*@@@+++@@@@******************************************************************
//
// Copyright (c) Microsoft Corp.
// All rights reserved.
//
//*@@@---@@@@******************************************************************

#ifndef JXR_POST_PROCESS_BLOCK_EDGE_APPLIER_H
#define JXR_POST_PROCESS_BLOCK_EDGE_APPLIER_H

#include "windowsmediaphoto.h"

/* Applies all four smoothing lines of a horizontal 4x4-block boundary. */
Void JxrPostProcessBlockEdgeApplierApplyHorizontal(
    PixelI* previousMacroblockRow,
    PixelI* currentMacroblockRow,
    size_t blockRow,
    size_t blockColumn);

/* Applies all four smoothing lines of a vertical 4x4-block boundary. */
Void JxrPostProcessBlockEdgeApplierApplyVertical(
    PixelI* previousMacroblockRow,
    size_t blockRow,
    size_t blockColumn);

#endif
