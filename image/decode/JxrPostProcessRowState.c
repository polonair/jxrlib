//*@@@+++@@@@******************************************************************
//
// Copyright (c) Microsoft Corp.
// All rights reserved.
//
//*@@@---@@@@******************************************************************

#include "JxrPostProcessRowState.h"

#include <assert.h>
#include <stdlib.h>

static Void JxrPostProcessRowStateInitializeBoundary(struct tagPostProcInfo* row)
{
    size_t blockX;
    size_t blockY;

    row[-1].ucMBTexture = 3;
    for (blockY = 0; blockY < 4; ++blockY) {
        for (blockX = 0; blockX < 4; ++blockX) {
            row[-1].ucBlockTexture[blockY][blockX] = 3;
        }
    }
}

Int JxrPostProcessRowStateInitialize(
    struct tagPostProcInfo* rows[MAX_CHANNELS][2],
    size_t macroblockWidth,
    size_t channelCount)
{
    size_t channel;
    size_t rowIndex;
    Bool is32Bit = sizeof(int) == 4;

    for (channel = 0; channel < channelCount; ++channel) {
        for (rowIndex = 0; rowIndex < 2; ++rowIndex) {
            if (is32Bit && ((((macroblockWidth + 2) >> 16) * sizeof(struct tagPostProcInfo)) & 0xffff0000)) {
                return ICERR_ERROR;
            }

            rows[channel][rowIndex] = (struct tagPostProcInfo*)malloc(
                (macroblockWidth + 2) * sizeof(struct tagPostProcInfo));
            assert(rows[channel][rowIndex] != NULL);
            if (rows[channel][rowIndex] == NULL) {
                return ICERR_ERROR;
            }

            ++rows[channel][rowIndex];
            JxrPostProcessRowStateInitializeBoundary(rows[channel][rowIndex]);
            rows[channel][rowIndex][macroblockWidth] = rows[channel][rowIndex][-1];
        }
    }

    return ICERR_OK;
}

Void JxrPostProcessRowStateRelease(
    struct tagPostProcInfo* rows[MAX_CHANNELS][2],
    size_t channelCount)
{
    size_t channel;
    size_t rowIndex;

    for (channel = 0; channel < channelCount; ++channel) {
        for (rowIndex = 0; rowIndex < 2; ++rowIndex) {
            if (rows[channel][rowIndex] != NULL) {
                free(rows[channel][rowIndex] - 1);
            }
        }
    }
}

Void JxrPostProcessRowStateAdvance(
    struct tagPostProcInfo* rows[MAX_CHANNELS][2],
    size_t channelCount,
    size_t macroblockWidth,
    Bool isTopRow,
    Bool isBottomRow)
{
    size_t channel;
    size_t macroblockIndex;

    for (channel = 0; channel < channelCount; ++channel) {
        struct tagPostProcInfo* previousRow = rows[channel][0];
        rows[channel][0] = rows[channel][1];
        rows[channel][1] = previousRow;

        if (isTopRow) {
            for (macroblockIndex = 0; macroblockIndex < macroblockWidth; ++macroblockIndex) {
                rows[channel][0][macroblockIndex] = rows[channel][0][-1];
            }
        }

        if (isBottomRow) {
            for (macroblockIndex = 0; macroblockIndex < macroblockWidth; ++macroblockIndex) {
                rows[channel][1][macroblockIndex] = rows[channel][1][-1];
            }
        }
    }
}
