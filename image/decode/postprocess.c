//*@@@+++@@@@******************************************************************
//
// Copyright © Microsoft Corp.
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 
// • Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
// • Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
//*@@@---@@@@******************************************************************

#include "windowsmediaphoto.h"
#include "strcodec.h"
#include "JxrPostProcessDecision.h"
#include "JxrPostProcessRowState.h"
#include "JxrPostProcessBlockNeighborhood.h"
#include "JxrPostProcessSmoothing.h"
#include "JxrPostProcessMacroblockAnalyzer.h"
#include "JxrPostProcessBlockDcCollector.h"
#include "JxrPostProcessMacroblockNeighborhood.h"
#include "JxrPostProcessBlockEdgeApplier.h"

Int initPostProc(struct tagPostProcInfo * strPostProcInfo[MAX_CHANNELS][2], size_t mbWidth, size_t iNumChannels)
{
    return JxrPostProcessRowStateInitialize(strPostProcInfo, mbWidth, iNumChannels);
}

Void termPostProc(struct tagPostProcInfo * strPostProcInfo[MAX_CHANNELS][2], size_t iNumChannels)
{
    JxrPostProcessRowStateRelease(strPostProcInfo, iNumChannels);
}

Void slideOneMBRow(struct tagPostProcInfo * strPostProcInfo[MAX_CHANNELS][2], size_t iNumChannels, size_t mbWidth, Bool top, Bool bottom)
{
    JxrPostProcessRowStateAdvance(strPostProcInfo, iNumChannels, mbWidth, top, bottom);
}
// get DC and texture infomation right before transform
Void updatePostProcInfo(struct tagPostProcInfo * strPostProcInfo[MAX_CHANNELS][2], PixelI * pMB, size_t mbX, size_t cc)
{
    struct tagPostProcInfo * pMBInfo = strPostProcInfo[cc][1] + mbX;

    JxrPostProcessMacroblockAnalyzerAnalyze(pMB, pMBInfo);
}
// demacroblock and get DCs of blocks
Void postProcMB(struct tagPostProcInfo * strPostProcInfo[MAX_CHANNELS][2], PixelI * p0, PixelI * p1, size_t mbX, size_t cc, Int threshold)
{
    /* 4 MBs involved, current MB is d, we have 4 2-pixel boundary segments */
    /*    |     */
    /*  a | b   */
    /* - - + +  */
    /*  c ! d   */
    /*    !     */
    JxrPostProcessMacroblockNeighborhood macroblocks;

    JxrPostProcessMacroblockNeighborhoodLoad(&macroblocks, strPostProcInfo, cc, mbX);

    // demacroblock segment --
   if(JxrPostProcessShouldDemacroblock(macroblocks.topLeft, macroblocks.bottomLeft, threshold)){
        JxrPostProcessSmoothingApplyMacroblockEdge(p0 - 256 + 10 * 16, p0 - 256 + 11 * 16, p1 - 256 +  8 * 16, p1 - 256 +  9 * 16);
        JxrPostProcessSmoothingApplyMacroblockEdge(p0 - 256 + 14 * 16, p0 - 256 + 15 * 16, p1 - 256 + 12 * 16, p1 - 256 + 13 * 16);
    }

    // demacroblock segment ++
    if(JxrPostProcessShouldDemacroblock(macroblocks.topRight, macroblocks.bottomRight, threshold)){
        JxrPostProcessSmoothingApplyMacroblockEdge(p0 + 2 * 16, p0 + 3 * 16, p1 + 0 * 16, p1 + 1 * 16);
        JxrPostProcessSmoothingApplyMacroblockEdge(p0 + 6 * 16, p0 + 7 * 16, p1 + 4 * 16, p1 + 5 * 16);
    }

    // demacroblock segment |
    if(JxrPostProcessShouldDemacroblock(macroblocks.topLeft, macroblocks.topRight, threshold)){
        JxrPostProcessSmoothingApplyMacroblockEdge(p0 - 256 + 10 * 16, p0 - 256 + 14 * 16, p0 + 2 * 16, p0 + 6 * 16);
        JxrPostProcessSmoothingApplyMacroblockEdge(p0 - 256 + 11 * 16, p0 - 256 + 15 * 16, p0 + 3 * 16, p0 + 7 * 16);
    }

    // demacroblock segment !
    if(JxrPostProcessShouldDemacroblock(macroblocks.bottomLeft, macroblocks.bottomRight, threshold)){
        JxrPostProcessSmoothingApplyMacroblockEdge(p1 - 256 + 8 * 16, p1 - 256 + 12 * 16, p1 + 0 * 16, p1 + 4 * 16);
        JxrPostProcessSmoothingApplyMacroblockEdge(p1 - 256 + 9 * 16, p1 - 256 + 13 * 16, p1 + 1 * 16, p1 + 5 * 16);
    }

    JxrPostProcessBlockDcCollectorCollect(p0, p1, macroblocks.topLeft, macroblocks.topRight, macroblocks.bottomLeft, macroblocks.bottomRight);
}

/* deblock and destair blocks */
/* 4 MBs involved, need to process 16 blocks of a */
/*    |     */
/*  a | b   */
/* - - - -  */
/*  c | d   */
/*    |     */
Void postProcBlock(struct tagPostProcInfo * strPostProcInfo[MAX_CHANNELS][2], PixelI * p0, PixelI * p1, size_t mbX, size_t cc, Int threshold)
{
    size_t i, j;
    JxrPostProcessBlockNeighborhood neighborhood;
    JxrPostProcessMacroblockNeighborhood macroblocks;

    JxrPostProcessMacroblockNeighborhoodLoad(&macroblocks, strPostProcInfo, cc, mbX);

    JxrPostProcessBlockNeighborhoodLoad(&neighborhood, macroblocks.topLeft, macroblocks.topRight, macroblocks.bottomLeft, macroblocks.bottomRight);

    /* block boundaries */
    /*     | */
    /*     | */
    /*  ---  */

    for(j = 0; j < 4; j ++){
        for(i = 0; i < 4; i ++){
            if(JxrPostProcessBlockNeighborhoodShouldSmoothHorizontal(&neighborhood, j, i, threshold)){
                JxrPostProcessBlockEdgeApplierApplyHorizontal(p0, p1, j, i);
            }

            if(JxrPostProcessBlockNeighborhoodShouldSmoothVertical(&neighborhood, j, i, threshold)){
                JxrPostProcessBlockEdgeApplierApplyVertical(p0, j, i);
            }
        }
    }
}

