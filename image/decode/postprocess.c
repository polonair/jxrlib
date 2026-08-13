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
    size_t i, j;
    struct tagPostProcInfo * pMBInfo = strPostProcInfo[cc][1] + mbX;

    // DC of MB
    pMBInfo->iMBDC = pMB[0];

    // texture of MB
    pMBInfo->ucMBTexture = 0; // smooth
    for(i = 16; i < 256; i += 16){
        if(pMB[i] != 0){
            pMBInfo->ucMBTexture = 3; // bumpy
            break;
        }
    }

    // DCs of blocks not available yet, will collect after demacroblocking

    // textures of blocks
    for(j = 0; j < 4; j ++)
        for(i = 0; i < 4; i ++){
            PixelI * p = pMB + i * 64 + j * 16;
            size_t k;

            for(k = 1, pMBInfo->ucBlockTexture[j][i] = 0; k < 16; k ++){
                if(p[k] != 0){
                    pMBInfo->ucBlockTexture[j][i] = 3;
                    break;
                }
            }
        }
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
    struct tagPostProcInfo * pMBb = strPostProcInfo[cc][0] + mbX, * pMBa = pMBb - 1, * pMBd = strPostProcInfo[cc][1] + mbX, * pMBc = pMBd - 1;

    // demacroblock segment --
   if(JxrPostProcessShouldDemacroblock(pMBa, pMBc, threshold)){
        JxrPostProcessSmoothingApplyMacroblockEdge(p0 - 256 + 10 * 16, p0 - 256 + 11 * 16, p1 - 256 +  8 * 16, p1 - 256 +  9 * 16);
        JxrPostProcessSmoothingApplyMacroblockEdge(p0 - 256 + 14 * 16, p0 - 256 + 15 * 16, p1 - 256 + 12 * 16, p1 - 256 + 13 * 16);
    }

    // demacroblock segment ++
    if(JxrPostProcessShouldDemacroblock(pMBb, pMBd, threshold)){
        JxrPostProcessSmoothingApplyMacroblockEdge(p0 + 2 * 16, p0 + 3 * 16, p1 + 0 * 16, p1 + 1 * 16);
        JxrPostProcessSmoothingApplyMacroblockEdge(p0 + 6 * 16, p0 + 7 * 16, p1 + 4 * 16, p1 + 5 * 16);
    }

    // demacroblock segment |
    if(JxrPostProcessShouldDemacroblock(pMBa, pMBb, threshold)){
        JxrPostProcessSmoothingApplyMacroblockEdge(p0 - 256 + 10 * 16, p0 - 256 + 14 * 16, p0 + 2 * 16, p0 + 6 * 16);
        JxrPostProcessSmoothingApplyMacroblockEdge(p0 - 256 + 11 * 16, p0 - 256 + 15 * 16, p0 + 3 * 16, p0 + 7 * 16);
    }

    // demacroblock segment !
    if(JxrPostProcessShouldDemacroblock(pMBc, pMBd, threshold)){
        JxrPostProcessSmoothingApplyMacroblockEdge(p1 - 256 + 8 * 16, p1 - 256 + 12 * 16, p1 + 0 * 16, p1 + 4 * 16);
        JxrPostProcessSmoothingApplyMacroblockEdge(p1 - 256 + 9 * 16, p1 - 256 + 13 * 16, p1 + 1 * 16, p1 + 5 * 16);
    }

    /* update DCs of blocks */
    // MB d 
    pMBd->iBlockDC[0][0] = p1[0 * 16];
    pMBd->iBlockDC[0][1] = p1[4 * 16];
    pMBd->iBlockDC[1][0] = p1[1 * 16];
    pMBd->iBlockDC[1][1] = p1[5 * 16];
    
    // MB b
    pMBb->iBlockDC[2][0] = p0[2 * 16];
    pMBb->iBlockDC[2][1] = p0[6 * 16];
    pMBb->iBlockDC[3][0] = p0[3 * 16];
    pMBb->iBlockDC[3][1] = p0[7 * 16];

    // MB c
    pMBc->iBlockDC[0][2] = p1[ 8 * 16 - 256];
    pMBc->iBlockDC[0][3] = p1[12 * 16 - 256];
    pMBc->iBlockDC[1][2] = p1[ 9 * 16 - 256];
    pMBc->iBlockDC[1][3] = p1[13 * 16 - 256];

    // MB a
    pMBa->iBlockDC[2][2] = p0[10 * 16 - 256];
    pMBa->iBlockDC[2][3] = p0[14 * 16 - 256];
    pMBa->iBlockDC[3][2] = p0[11 * 16 - 256];
    pMBa->iBlockDC[3][3] = p0[15 * 16 - 256];
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
    size_t i, j, k;
    JxrPostProcessBlockNeighborhood neighborhood;
    struct tagPostProcInfo * pMBb = strPostProcInfo[cc][0] + mbX, * pMBa = pMBb - 1, * pMBd = strPostProcInfo[cc][1] + mbX, * pMBc = pMBd - 1;
    PixelI * pc, * pt;

    JxrPostProcessBlockNeighborhoodLoad(&neighborhood, pMBa, pMBb, pMBc, pMBd);

    /* block boundaries */
    /*     | */
    /*     | */
    /*  ---  */

    for(j = 0; j < 4; j ++){
        for(i = 0; i < 4; i ++){
            pc = p0 - 256 + i * 64 + j * 16;

            // deblock
            if(JxrPostProcessBlockNeighborhoodShouldSmoothHorizontal(&neighborhood, j, i, threshold)){
                // smooth horizontal boundary ----
                pt = (j < 3 ? pc + 16 : p1 - 256 + i * 64);
                for(k = 0; k < 4; k ++){
                    JxrPostProcessSmoothingApplyBlockEdge(pc + idxCC[1][k], pc + idxCC[2][k], pc + idxCC[3][k], pt + idxCC[0][k], pt + idxCC[1][k], pt + idxCC[2][k]);
                }
            }

            // two horizontally adjacent blocks have same texture and similiar DCs
            if(JxrPostProcessBlockNeighborhoodShouldSmoothVertical(&neighborhood, j, i, threshold)){
                // smooth vertical boundary |
                pt = pc + 64;
                for(k = 0; k < 4; k ++){
                    JxrPostProcessSmoothingApplyBlockEdge(pc + idxCC[k][1], pc + idxCC[k][2], pc + idxCC[k][3], pt + idxCC[k][0], pt + idxCC[k][1], pt + idxCC[k][2]);
                }
            }
        }
    }
}

