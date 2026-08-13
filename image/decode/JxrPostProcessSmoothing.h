//*@@@+++@@@@******************************************************************
//
// Copyright (c) Microsoft Corp.
// All rights reserved.
//
//*@@@---@@@@******************************************************************

#ifndef JXR_POST_PROCESS_SMOOTHING_H
#define JXR_POST_PROCESS_SMOOTHING_H

#include "windowsmediaphoto.h"

/* Applies the four-sample smoothing rule across a macroblock edge. */
Void JxrPostProcessSmoothingApplyMacroblockEdge(
    PixelI* leftOuter,
    PixelI* leftInner,
    PixelI* rightInner,
    PixelI* rightOuter);

/* Applies the six-sample smoothing rule across a 4x4 block edge. */
Void JxrPostProcessSmoothingApplyBlockEdge(
    PixelI* leftFar,
    PixelI* leftOuter,
    PixelI* leftInner,
    PixelI* rightInner,
    PixelI* rightOuter,
    PixelI* rightFar);

#endif
