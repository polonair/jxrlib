//*@@@+++@@@@******************************************************************
//
// Copyright (c) Microsoft Corp.
// All rights reserved.
//
//*@@@---@@@@******************************************************************

#include "JxrPostProcessDecision.h"

#include <stdlib.h>

Bool JxrPostProcessShouldDemacroblock(
    const struct tagPostProcInfo* first,
    const struct tagPostProcInfo* second,
    Int threshold)
{
    Int dcDifference;

    if (first->ucMBTexture + second->ucMBTexture != 0) {
        return FALSE;
    }

    dcDifference = first->iMBDC - second->iMBDC;
    if (dcDifference < 0) {
        dcDifference = -dcDifference;
    }

    return dcDifference <= threshold;
}
