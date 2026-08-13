//*@@@+++@@@@******************************************************************
//
// Copyright (c) Microsoft Corp.
// All rights reserved.
//
//*@@@---@@@@******************************************************************

#include "JxrPostProcessDecision.h"

static Int JxrPostProcessAbsoluteDcDifference(Int firstDc, Int secondDc)
{
    Int difference = firstDc - secondDc;

    if (difference < 0) {
        difference = -difference;
    }

    return difference;
}

Bool JxrPostProcessShouldDemacroblock(
    const struct tagPostProcInfo* first,
    const struct tagPostProcInfo* second,
    Int threshold)
{
    if (first->ucMBTexture + second->ucMBTexture != 0) {
        return FALSE;
    }

    return JxrPostProcessAbsoluteDcDifference(first->iMBDC, second->iMBDC) <= threshold;
}

Bool JxrPostProcessShouldDeblockBoundary(
    U8 firstTexture,
    Int firstDc,
    U8 secondTexture,
    Int secondDc,
    Int threshold)
{
    return firstTexture + secondTexture < 3 &&
        JxrPostProcessAbsoluteDcDifference(firstDc, secondDc) <= threshold;
}
