//*@@@+++@@@@******************************************************************
//
// Copyright (c) Microsoft Corp.
// All rights reserved.
//
//*@@@---@@@@******************************************************************

#include "JxrPostProcessSmoothing.h"

Void JxrPostProcessSmoothingApplyMacroblockEdge(
    PixelI* leftOuter,
    PixelI* leftInner,
    PixelI* rightInner,
    PixelI* rightOuter)
{
    PixelI delta = (((*rightInner - *leftInner) << 2) +
        (*leftOuter - *rightOuter)) >> 3;

    *rightInner -= delta;
    *leftInner += delta;
}

Void JxrPostProcessSmoothingApplyBlockEdge(
    PixelI* leftFar,
    PixelI* leftOuter,
    PixelI* leftInner,
    PixelI* rightInner,
    PixelI* rightOuter,
    PixelI* rightFar)
{
    PixelI delta = (((*rightInner - *leftInner) << 2) +
        (*leftOuter - *rightOuter)) >> 3;

    *rightInner -= delta;
    *leftInner += delta;
    *leftOuter = (*leftOuter >> 1) + ((*leftInner + *leftFar) >> 2);
    *rightOuter = (*rightOuter >> 1) + ((*rightInner + *rightFar) >> 2);
}
