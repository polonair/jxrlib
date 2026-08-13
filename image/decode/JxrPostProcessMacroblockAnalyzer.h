//*@@@+++@@@@******************************************************************
//
// Copyright (c) Microsoft Corp.
// All rights reserved.
//
//*@@@---@@@@******************************************************************

#ifndef JXR_POST_PROCESS_MACROBLOCK_ANALYZER_H
#define JXR_POST_PROCESS_MACROBLOCK_ANALYZER_H

#include "strcodec.h"

/* Extracts post-processing DC and texture information from one 16x16 macroblock. */
Void JxrPostProcessMacroblockAnalyzerAnalyze(
    const PixelI* coefficients,
    struct tagPostProcInfo* result);

#endif
