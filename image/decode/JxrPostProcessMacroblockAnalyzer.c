//*@@@+++@@@@******************************************************************
//
// Copyright (c) Microsoft Corp.
// All rights reserved.
//
//*@@@---@@@@******************************************************************

#include "JxrPostProcessMacroblockAnalyzer.h"

Void JxrPostProcessMacroblockAnalyzerAnalyze(
    const PixelI* coefficients,
    struct tagPostProcInfo* result)
{
    size_t coefficientIndex;
    size_t blockRow;
    size_t blockColumn;

    result->iMBDC = coefficients[0];
    result->ucMBTexture = 0;
    for (coefficientIndex = 16; coefficientIndex < 256; coefficientIndex += 16) {
        if (coefficients[coefficientIndex] != 0) {
            result->ucMBTexture = 3;
            break;
        }
    }

    for (blockRow = 0; blockRow < 4; ++blockRow) {
        for (blockColumn = 0; blockColumn < 4; ++blockColumn) {
            const PixelI* block = coefficients + blockColumn * 64 + blockRow * 16;
            size_t blockCoefficientIndex;

            result->ucBlockTexture[blockRow][blockColumn] = 0;
            for (blockCoefficientIndex = 1; blockCoefficientIndex < 16; ++blockCoefficientIndex) {
                if (block[blockCoefficientIndex] != 0) {
                    result->ucBlockTexture[blockRow][blockColumn] = 3;
                    break;
                }
            }
        }
    }
}
