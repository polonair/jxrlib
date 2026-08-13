//*@@@+++@@@@******************************************************************
//
// Copyright (c) Microsoft Corp.
// All rights reserved.
//
//*@@@---@@@@******************************************************************

#ifndef JXR_POST_PROCESS_ROW_STATE_H
#define JXR_POST_PROCESS_ROW_STATE_H

#include "strcodec.h"

/*
 * Owns the two post-processing macroblock rows for each image channel.
 * Each exposed row begins after its left boundary element; the right boundary
 * is at index macroblockWidth.
 */
Int JxrPostProcessRowStateInitialize(
    struct tagPostProcInfo* rows[MAX_CHANNELS][2],
    size_t macroblockWidth,
    size_t channelCount);

Void JxrPostProcessRowStateRelease(
    struct tagPostProcInfo* rows[MAX_CHANNELS][2],
    size_t channelCount);

Void JxrPostProcessRowStateAdvance(
    struct tagPostProcInfo* rows[MAX_CHANNELS][2],
    size_t channelCount,
    size_t macroblockWidth,
    Bool isTopRow,
    Bool isBottomRow);

#endif
