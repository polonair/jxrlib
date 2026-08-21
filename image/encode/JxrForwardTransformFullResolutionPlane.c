#include "JxrForwardTransformFullResolutionPlane.h"

#include "encode.h"

Void JxrForwardTransformFullResolutionPlaneApply(
    PixelI* firstStage,
    PixelI* secondStage,
    Bool isChroma,
    const JxrForwardTransformMacroblockGeometry* geometry,
    const JxrForwardTransformBoundaryContext* boundaries,
    Bool usesScaledArithmetic)
{
    PixelI* samples;
    Int offset;

    if (geometry->overlap != OL_NONE) {
        if (boundaries->hasTopBoundary && boundaries->hasLeftBoundary)
            strPre4(secondStage + 0, secondStage + 1, secondStage + 2, secondStage + 3);
        if (boundaries->hasTopBoundary && boundaries->hasRightBoundary)
            strPre4(secondStage - 59, secondStage - 60, secondStage - 57, secondStage - 58);
        if (boundaries->hasBottomBoundary && boundaries->hasLeftBoundary)
            strPre4(firstStage + 58, firstStage + 59, firstStage + 56, firstStage + 57);
        if (boundaries->hasBottomBoundary && boundaries->hasRightBoundary)
            strPre4(firstStage - 1, firstStage - 2, firstStage - 3, firstStage - 4);
        if (!geometry->isRight && !geometry->isBottom) {
            if (boundaries->hasTopBoundary) {
                for (offset = boundaries->hasLeftBoundary ? 0 : -64; offset < 192; offset += 64) {
                    samples = secondStage + offset;
                    strPre4(samples + 5, samples + 4, samples + 64, samples + 65);
                    strPre4(samples + 7, samples + 6, samples + 66, samples + 67);
                }
            }
            else {
                for (offset = boundaries->hasLeftBoundary ? 0 : -64; offset < 192; offset += 64)
                    strPre4x4Stage1Split(firstStage + 48 + offset, secondStage + offset, 0);
            }

            if (boundaries->hasLeftBoundary) {
                if (!geometry->isTop && !boundaries->isHorizontalTileBoundary) {
                    strPre4(firstStage + 58, firstStage + 56, secondStage + 0, secondStage + 2);
                    strPre4(firstStage + 59, firstStage + 57, secondStage + 1, secondStage + 3);
                }

                for (offset = -64; offset < -16; offset += 16) {
                    samples = secondStage + offset;
                    strPre4(samples + 74, samples + 72, samples + 80, samples + 82);
                    strPre4(samples + 75, samples + 73, samples + 81, samples + 83);
                }
            }
            else {
                for (offset = -64; offset < -16; offset += 16)
                    strPre4x4Stage1(secondStage + offset, 0);
            }

            strPre4x4Stage1(secondStage + 0, 0);
            strPre4x4Stage1(secondStage + 16, 0);
            strPre4x4Stage1(secondStage + 32, 0);
            strPre4x4Stage1(secondStage + 64, 0);
            strPre4x4Stage1(secondStage + 80, 0);
            strPre4x4Stage1(secondStage + 96, 0);
            strPre4x4Stage1(secondStage + 128, 0);
            strPre4x4Stage1(secondStage + 144, 0);
            strPre4x4Stage1(secondStage + 160, 0);
        }

        if (boundaries->hasBottomBoundary) {
            for (offset = boundaries->hasLeftBoundary ? 48 : -16;
                offset < (geometry->isRight ? -16 : 240); offset += 64) {
                samples = firstStage + offset;
                strPre4(samples + 15, samples + 14, samples + 74, samples + 75);
                strPre4(samples + 13, samples + 12, samples + 72, samples + 73);
            }
        }

        if (boundaries->hasRightBoundary && !geometry->isBottom) {
            if (!geometry->isTop && !boundaries->isHorizontalTileBoundary) {
                strPre4(firstStage - 1, firstStage - 3, secondStage - 59, secondStage - 57);
                strPre4(firstStage - 2, firstStage - 4, secondStage - 60, secondStage - 58);
            }
            for (offset = -64; offset < -16; offset += 16) {
                samples = secondStage + offset;
                strPre4(samples + 15, samples + 13, samples + 21, samples + 23);
                strPre4(samples + 14, samples + 12, samples + 20, samples + 22);
            }
        }
    }

    if (!geometry->isTop) {
        for (offset = geometry->isLeft ? 48 : -16;
            offset < (geometry->isRight ? 48 : 240); offset += 64)
            strDCT4x4Stage1(firstStage + offset);
    }

    if (!geometry->isBottom) {
        for (offset = geometry->isLeft ? 0 : -64;
            offset < (geometry->isRight ? 0 : 192); offset += 64) {
            strDCT4x4Stage1(secondStage + offset + 0);
            strDCT4x4Stage1(secondStage + offset + 16);
            strDCT4x4Stage1(secondStage + offset + 32);
        }
    }

    if (geometry->overlap == OL_TWO) {
        if (boundaries->hasTopBoundary && boundaries->hasLeftBoundary)
            strPre4(secondStage + 0, secondStage + 64, secondStage + 16, secondStage + 80);
        if (boundaries->hasTopBoundary && boundaries->hasRightBoundary)
            strPre4(secondStage - 128, secondStage - 64, secondStage - 112, secondStage - 48);
        if (boundaries->hasBottomBoundary && boundaries->hasLeftBoundary)
            strPre4(firstStage + 32, firstStage + 96, firstStage + 48, firstStage + 112);
        if (boundaries->hasBottomBoundary && boundaries->hasRightBoundary)
            strPre4(firstStage - 96, firstStage - 32, firstStage - 80, firstStage - 16);
        if (boundaries->hasLeftOrRightBoundary && !boundaries->hasTopOrBottomBoundary) {
            if (boundaries->hasLeftBoundary) {
                strPre4(firstStage + 32, firstStage + 48, secondStage + 0, secondStage + 16);
                strPre4(firstStage + 96, firstStage + 112, secondStage + 64, secondStage + 80);
            }
            if (boundaries->hasRightBoundary) {
                strPre4(firstStage - 96, firstStage - 80, secondStage - 128, secondStage - 112);
                strPre4(firstStage - 32, firstStage - 16, secondStage - 64, secondStage - 48);
            }
        }

        if (!boundaries->hasLeftOrRightBoundary) {
            if (boundaries->hasTopOrBottomBoundary) {
                if (boundaries->hasTopBoundary) {
                    strPre4(secondStage - 128, secondStage - 64, secondStage + 0, secondStage + 64);
                    strPre4(secondStage - 112, secondStage - 48, secondStage + 16, secondStage + 80);
                }
                if (boundaries->hasBottomBoundary) {
                    samples = firstStage + 32;
                    strPre4(samples - 128, samples - 64, samples + 0, samples + 64);
                    strPre4(samples - 112, samples - 48, samples + 16, samples + 80);
                }
            }
            else {
                strPre4x4Stage2Split(firstStage, secondStage);
            }
        }
    }

    if (!geometry->isTopOrLeft) {
        if (usesScaledArithmetic)
            strNormalizeEnc(firstStage - 256, isChroma);
        strDCT4x4SecondStage(firstStage - 256);
    }
}
