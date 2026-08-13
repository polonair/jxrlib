//*@@@+++@@@@******************************************************************
//
// Copyright (c) Microsoft Corp.
// All rights reserved.
//
//*@@@---@@@@******************************************************************

#ifndef JXR_POST_PROCESS_DECISION_H
#define JXR_POST_PROCESS_DECISION_H

#include "strcodec.h"

/*
 * Returns TRUE when two adjacent macroblocks can be demacroblocked.
 * Both macroblocks must be smooth and their DC values must differ by no more
 * than the caller-supplied threshold.
 */
Bool JxrPostProcessShouldDemacroblock(
    const struct tagPostProcInfo* first,
    const struct tagPostProcInfo* second,
    Int threshold);

#endif
