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

#include "strTransform.h"
#include "strcodec.h"
#include "decode.h"
#include "JxrInverseTransformMath.h"
#include "JxrTransformMath.h"

/** local functions **/

/** IDCT stuff **/
/** reordering should be combined with zigzag scan **/
/** data order before IDCT **/
/** 0  8  4  6 **/
/** 2 10 14 12 **/
/** 1 11 15 13 **/
/** 9  3  7  5 **/
/** data order after IDCT **/
/**  0  1  2  3 **/
/**  4  5  6  7 **/
/**  8  9 10 11 **/
/** 12 13 14 15 **/
Void strIDCT4x4Stage1(PixelI* p)
{
    /** top left corner, butterfly => butterfly **/
    JxrTransformMathApplyDct2x2Up(p + 0, p + 1, p + 2, p + 3);

    /** top right corner, -pi/8 rotation => butterfly **/
    JxrInverseTransformMathApplyOdd(p + 5, p + 4, p + 7, p + 6);

    /** bottom left corner, butterfly => -pi/8 rotation **/
    JxrInverseTransformMathApplyOdd(p + 10, p + 8, p + 11, p + 9);

    /** bottom right corner, -pi/8 rotation => -pi/8 rotation **/
    JxrInverseTransformMathApplyOddOdd(p + 15, p + 14, p + 13, p + 12);
    
    /** butterfly **/
    //FOURBUTTERFLY(p, 0, 4, 8, 12, 1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15);
    JxrTransformMathApplyFourButterfly(p, JxrTransformFirstStageFourButterflyOffsets);
}

Void strIDCT4x4Stage2(PixelI* p)
{
    /** bottom left corner, butterfly => -pi/8 rotation **/
    JxrInverseTransformMathApplyOdd(p + 32, p + 48, p + 96, p + 112);
    
    /** top right corner, -pi/8 rotation => butterfly **/
    JxrInverseTransformMathApplyOdd(p + 128, p + 192, p + 144, p + 208);
    
    /** bottom right corner, -pi/8 rotation => -pi/8 rotation **/
    JxrInverseTransformMathApplyOddOdd(p + 160, p + 224, p + 176, p + 240);

    /** top left corner, butterfly => butterfly **/
    JxrTransformMathApplyDct2x2Up(p + 0, p + 64, p + 16, p + 80);
    
    /** butterfly **/
    JxrTransformMathApplyFourButterfly(p, JxrTransformSecondStageFourButterflyOffsets);
}

Void strNormalizeDec(PixelI* p, Bool bChroma)
{
    int i;
    if (!bChroma) {
        //for (i = 0; i < 256; i += 16) {
        //    p[i] <<= 2;
        //}
    }
    else {
        for (i = 0; i < 256; i += 16) {
            p[i] += p[i];
        }
    }
}

/*****************************************************************************************
  Input data offsets:
  (15)(14)|(10+64)(11+64) p0 (15)(14)|(74)(75)
  (13)(12)|( 8+64)( 9+64)    (13)(12)|(72)(73)
  --------+--------------    --------+--------
  ( 5)( 4)|( 0+64) (1+64) p1 ( 5)( 4)|(64)(65)
  ( 7)( 6)|( 2+64) (3+64)    ( 7)( 6)|(66)(67)
*****************************************************************************************/
Void strPost4x4Stage1Split(PixelI *p0, PixelI *p1, Int iOffset, Int iHPQP, Bool bHPAbsent)
{
    Int column;
    Int directCurrent[4];
    Int temporaryCurrent;
    PixelI *p2 = p0 + 72 - iOffset;
    PixelI *p3 = p1 + 64 - iOffset;

    p0 += 12;
    p1 += 4;

    /* Apply the 2x2 DCT to each of the four aligned columns. */
    for (column = 0; column < 4; ++column) {
        JxrTransformMathApplyDct2x2Down(p0 + column, p2 + column, p1 + column, p3 + column);
    }

    /* Transform the bottom-right corner as one 4-point operation. */
    JxrInverseTransformMathApplyOddOddPost(p3 + 0, p3 + 1, p3 + 2, p3 + 3);

    /* Rotate the two anti-diagonal corners. */
    JxrInverseTransformMathRotateHalf(&p1[2], &p1[3]);
    JxrInverseTransformMathRotateHalf(&p1[0], &p1[1]);
    JxrInverseTransformMathRotateHalf(&p2[1], &p2[3]);
    JxrInverseTransformMathRotateHalf(&p2[0], &p2[2]);

    /* Complete the first and second Hadamard+scale passes for each column. */
    for (column = 0; column < 4; ++column) {
        JxrInverseTransformMathApplyHadamardScale2(p0 + column, p3 + column);
    }
    for (column = 0; column < 4; ++column) {
        JxrInverseTransformMathApplyHadamardScale4(p0 + column, p2 + column, p1 + column, p3 + column);
    }

    /* Compute all direct-current values before any compensation changes samples. */
    for (column = 0; column < 4; ++column) {
        temporaryCurrent = (p0[column] + p1[column] + p2[column] + p3[column]) >> 1;
        directCurrent[column] = (temporaryCurrent * 595 + 65536) >> 17;
    }

    /* Apply the optional direct-current compensation to each column. */
    for (column = 0; column < 4; ++column) {
        JxrInverseTransformMathApplyConditionalDcCompensation(
            p0 + column, p2 + column, p1 + column, p3 + column,
            directCurrent[column], iHPQP, bHPAbsent);
    }
}
Void strPost4x4Stage1(PixelI* p, Int iOffset, Int iHPQP, Bool bHPAbsent)
{
    strPost4x4Stage1Split(p, p + 16, iOffset, iHPQP, bHPAbsent);
}

Void strPost4x4Stage1Split_alternate(PixelI *p0, PixelI *p1, Int iOffset)
{
    Int column;
    PixelI *p2 = p0 + 72 - iOffset;
    PixelI *p3 = p1 + 64 - iOffset;

    p0 += 12;
    p1 += 4;

    /* Apply the 2x2 DCT to each of the four aligned columns. */
    for (column = 0; column < 4; ++column) {
        JxrTransformMathApplyDct2x2Down(p0 + column, p2 + column, p1 + column, p3 + column);
    }

    /* Transform the bottom-right corner as one 4-point operation. */
    JxrInverseTransformMathApplyOddOddPost(p3 + 0, p3 + 1, p3 + 2, p3 + 3);

    /* Rotate the two anti-diagonal corners. */
    JxrInverseTransformMathRotateHalf(&p1[2], &p1[3]);
    JxrInverseTransformMathRotateHalf(&p1[0], &p1[1]);
    JxrInverseTransformMathRotateHalf(&p2[1], &p2[3]);
    JxrInverseTransformMathRotateHalf(&p2[0], &p2[2]);

    /* Complete the alternate first and shared second Hadamard+scale passes. */
    for (column = 0; column < 4; ++column) {
        JxrInverseTransformMathApplyAlternateHadamardScale2(p0 + column, p3 + column);
    }
    for (column = 0; column < 4; ++column) {
        JxrInverseTransformMathApplyHadamardScale4(p0 + column, p2 + column, p1 + column, p3 + column);
    }
}
Void strPost4x4Stage1_alternate(PixelI* p, Int iOffset)
{
    strPost4x4Stage1Split_alternate(p, p + 16, iOffset);
}

/*****************************************************************************************
  Input data offsets:
  (15)(14)|(10+32)(11+32) p0 (15)(14)|(42)(43)
  (13)(12)|( 8+32)( 9+32)    (13)(12)|(40)(41)
  --------+--------------    --------+--------
  ( 5)( 4)|( 0+32) (1+32) p1 ( 5)( 4)|(32)(33)
  ( 7)( 6)|( 2+32) (3+32)    ( 7)( 6)|(34)(35)
*****************************************************************************************/

/*****************************************************************************************
  Input data offsets:
  ( -96)(-32)|(32)( 96) p0
  ( -80)(-16)|(48)(112)
  -----------+------------
  (-128)(-64)|( 0)( 64) p1
  (-112)(-48)|(16)( 80)
*****************************************************************************************/
Void strPost4x4Stage2Split(PixelI* p0, PixelI* p1)
{
    static const Int p0FirstOffsets[4] = { -96, -32, -80, -16 };
    static const Int p0SecondOffsets[4] = { 96, 32, 112, 48 };
    static const Int p1FirstOffsets[4] = { -112, -48, -128, -64 };
    static const Int p1SecondOffsets[4] = { 80, 16, 64, 0 };
    Int column;

    /* Apply the 2x2 DCT to each logical column in its legacy scan order. */
    for (column = 0; column < 4; ++column) {
        JxrTransformMathApplyDct2x2Down(
            p0 + p0FirstOffsets[column], p0 + p0SecondOffsets[column],
            p1 + p1FirstOffsets[column], p1 + p1SecondOffsets[column]);
    }

    /* Transform the bottom-right corner as one 4-point operation. */
    JxrInverseTransformMathApplyOddOddPost(p1 + 0, p1 + 64, p1 + 16, p1 + 80);

    /* Rotate the two anti-diagonal corners. */
    JxrInverseTransformMathRotateHalf(&p0[48], &p0[32]);
    JxrInverseTransformMathRotateHalf(&p0[112], &p0[96]);
    JxrInverseTransformMathRotateHalf(&p1[-64], &p1[-128]);
    JxrInverseTransformMathRotateHalf(&p1[-48], &p1[-112]);

    /* Complete the first and second Hadamard+scale passes in the same order. */
    for (column = 0; column < 4; ++column) {
        JxrInverseTransformMathApplyHadamardScale2(p0 + p0FirstOffsets[column], p1 + p1SecondOffsets[column]);
    }
    for (column = 0; column < 4; ++column) {
        JxrInverseTransformMathApplyHadamardScale4(
            p0 + p0FirstOffsets[column], p1 + p1FirstOffsets[column],
            p0 + p0SecondOffsets[column], p1 + p1SecondOffsets[column]);
    }
}
Void strPost4x4Stage2Split_alternate(PixelI* p0, PixelI* p1)
{
    static const Int p0FirstOffsets[4] = { -96, -32, -80, -16 };
    static const Int p0SecondOffsets[4] = { 96, 32, 112, 48 };
    static const Int p1FirstOffsets[4] = { -112, -48, -128, -64 };
    static const Int p1SecondOffsets[4] = { 80, 16, 64, 0 };
    Int column;

    /* Apply the 2x2 DCT to each logical column in its legacy scan order. */
    for (column = 0; column < 4; ++column) {
        JxrTransformMathApplyDct2x2Down(
            p0 + p0FirstOffsets[column], p0 + p0SecondOffsets[column],
            p1 + p1FirstOffsets[column], p1 + p1SecondOffsets[column]);
    }

    /* Transform the bottom-right corner as one 4-point operation. */
    JxrInverseTransformMathApplyOddOddPost(p1 + 0, p1 + 64, p1 + 16, p1 + 80);

    /* Rotate the two anti-diagonal corners. */
    JxrInverseTransformMathRotateHalf(&p0[48], &p0[32]);
    JxrInverseTransformMathRotateHalf(&p0[112], &p0[96]);
    JxrInverseTransformMathRotateHalf(&p1[-64], &p1[-128]);
    JxrInverseTransformMathRotateHalf(&p1[-48], &p1[-112]);

    /* Complete the alternate first and shared second Hadamard+scale passes. */
    for (column = 0; column < 4; ++column) {
        JxrInverseTransformMathApplyAlternateHadamardScale2(p0 + p0FirstOffsets[column], p1 + p1SecondOffsets[column]);
    }
    for (column = 0; column < 4; ++column) {
        JxrInverseTransformMathApplyHadamardScale4(
            p0 + p0FirstOffsets[column], p1 + p1FirstOffsets[column],
            p0 + p0SecondOffsets[column], p1 + p1SecondOffsets[column]);
    }
}
/*************************************************************************
  Top-level function to inverse tranform possible part of a macroblock
*************************************************************************/
Int  invTransformMacroblock(CWMImageStrCodec * pSC)
{
    const OVERLAP olOverlap = pSC->WMISCP.olOverlap;
    const COLORFORMAT cfColorFormat = pSC->m_param.cfColorFormat;
    // const BITDEPTH_BITS bdBitDepth = pSC->WMII.bdBitDepth;
    const Bool left = (pSC->cColumn == 0), right = (pSC->cColumn == pSC->cmbWidth);
    const Bool top = (pSC->cRow == 0), bottom = (pSC->cRow == pSC->cmbHeight);
    const Bool topORbottom = (top || bottom), leftORright = (left || right);
    const Bool topORleft = (top || left), bottomORright = (bottom || right);
    const size_t mbWidth = pSC->cmbWidth, mbX = pSC->cColumn;
    PixelI * p = NULL;// * pt = NULL;
    size_t i;
    const size_t iChannels = (cfColorFormat == YUV_420 || cfColorFormat == YUV_422) ? 1 : pSC->m_param.cNumChannels;
    const size_t tScale = pSC->m_Dparam->cThumbnailScale;
    Int j = 0;

    Int qp[MAX_CHANNELS], dcqp[MAX_CHANNELS], iStrength = (1 << pSC->WMII.cPostProcStrength);
    // ERR_CODE result = ICERR_OK;

    Bool bHPAbsent = (pSC->WMISCP.sbSubband == SB_NO_HIGHPASS || pSC->WMISCP.sbSubband == SB_DC_ONLY);

    if(pSC->WMII.cPostProcStrength > 0){
        // threshold for post processing
        for(i = 0; i < iChannels; i ++){
            qp[i] = pSC->pTile[pSC->cTileColumn].pQuantizerLP[i][pSC->MBInfo.iQIndexLP].iQP * iStrength * (olOverlap == OL_NONE ? 2 : 1);
            dcqp[i] = pSC->pTile[pSC->cTileColumn].pQuantizerDC[i][0].iQP * iStrength;
        }

        if(left) // a new MB row
            slideOneMBRow(pSC->pPostProcInfo, pSC->m_param.cNumChannels, mbWidth, top, bottom);  // previous current row becomes previous row
    }

    //================================================================
    // 400_Y, 444_YUV
    for (i = 0; i < iChannels && tScale < 16; ++i)
    {
        PixelI* const p0 = pSC->p0MBbuffer[i];
        PixelI* const p1 = pSC->p1MBbuffer[i];

        Int iHPQP = 255;
        if (!bHPAbsent)
            iHPQP = pSC->pTile[pSC->cTileColumn].pQuantizerHP[i][pSC->MBInfo.iQIndexHP].iQP;

        //================================
        // second level inverse transform
        if (!bottomORright)
        {
            if(pSC->WMII.cPostProcStrength > 0)
                updatePostProcInfo(pSC->pPostProcInfo, p1, mbX, i); // update postproc info before IDCT

            strIDCT4x4Stage2(p1);
            if (pSC->m_param.bScaledArith) {
                strNormalizeDec(p1, (i != 0));
            }
        }

        //================================
        // second level inverse overlap
        if (OL_TWO == olOverlap)
        {
            if (leftORright && (!topORbottom))
            {
                j = left ? 0 : -128;
                JxrInverseTransformMathApplyPost4(p0 + j + 32, p0 + j +  48, p1 + j +  0, p1 + j + 16);
                JxrInverseTransformMathApplyPost4(p0 + j + 96, p0 + j + 112, p1 + j + 64, p1 + j + 80);
            }

            if (!leftORright)
            {
                if (topORbottom)
                {
                    p = top ? p1 : p0 + 32;
                    JxrInverseTransformMathApplyPost4(p - 128, p - 64, p +  0, p + 64);
                    JxrInverseTransformMathApplyPost4(p - 112, p - 48, p + 16, p + 80);
                    p = NULL;
                }
                else
                {
                    strPost4x4Stage2Split(p0, p1);
                }
            }
        }

        if(pSC->WMII.cPostProcStrength > 0)
            postProcMB(pSC->pPostProcInfo, p0, p1, mbX, i, dcqp[i]); // second stage deblocking

        //================================
        // first level inverse transform
        if(tScale >= 4) // bypass first level transform for 4:1 and smaller thumbnail
            continue;

        if (!top)
        {
            for (j = (left ? 32 : -96); j < (right ? 32 : 160); j += 64)
            {
                strIDCT4x4Stage1(p0 + j +  0);
                strIDCT4x4Stage1(p0 + j + 16);
            }
        }

        if (!bottom)
        {
            for (j = (left ? 0 : -128); j < (right ? 0 : 128); j += 64)
            {
                strIDCT4x4Stage1(p1 + j +  0);
                strIDCT4x4Stage1(p1 + j + 16);
            }
        }

        //================================
        // first level inverse overlap
        if (OL_NONE != olOverlap)
        {
            if (leftORright)
            {
                j = left ? 0 + 10 : -64 + 14;
                if (!top)
                {
                    p = p0 + 16 + j;
                    JxrInverseTransformMathApplyPost4(p +  0, p -  2, p +  6, p +  8);
                    JxrInverseTransformMathApplyPost4(p +  1, p -  1, p +  7, p +  9);
                    JxrInverseTransformMathApplyPost4(p + 16, p + 14, p + 22, p + 24);
                    JxrInverseTransformMathApplyPost4(p + 17, p + 15, p + 23, p + 25);
                    p = NULL;
                }
                if (!bottom)
                {
                    p = p1 + j;
                    JxrInverseTransformMathApplyPost4(p + 0, p - 2, p + 6, p + 8);
                    JxrInverseTransformMathApplyPost4(p + 1, p - 1, p + 7, p + 9);
                    p = NULL;
                }
                if (!topORbottom)
                {
                    JxrInverseTransformMathApplyPost4(p0 + 48 + j + 0, p0 + 48 + j - 2, p1 - 10 + j, p1 - 8 + j);
                    JxrInverseTransformMathApplyPost4(p0 + 48 + j + 1, p0 + 48 + j - 1, p1 -  9 + j, p1 - 7 + j);
                }
            }

            if (top)
            {
                for (j = (left ? 0 : -192); j < (right ? -64 : 64); j += 64)
                {
                    p = p1 + j;
                    JxrInverseTransformMathApplyPost4(p + 5, p + 4, p + 64, p + 65);
                    JxrInverseTransformMathApplyPost4(p + 7, p + 6, p + 66, p + 67);
                    p = NULL;

                    strPost4x4Stage1(p1 + j, 0, iHPQP, bHPAbsent);
                }
            }
            else if (bottom)
            {
                for (j = (left ? 0 : -192); j < (right ? -64 : 64); j += 64)
                {
                    strPost4x4Stage1(p0 + 16 + j, 0, iHPQP, bHPAbsent);
                    strPost4x4Stage1(p0 + 32 + j, 0, iHPQP, bHPAbsent);

                    p = p0 + 48 + j;
                    JxrInverseTransformMathApplyPost4(p + 15, p + 14, p + 74, p + 75);
                    JxrInverseTransformMathApplyPost4(p + 13, p + 12, p + 72, p + 73);
                    p = NULL;
                }
            }
            else
            {
                for (j = (left ? 0 : -192); j < (right ? -64 : 64); j += 64)
                {
                    strPost4x4Stage1(p0 + 16 + j, 0, iHPQP, bHPAbsent);
                    strPost4x4Stage1(p0 + 32 + j, 0, iHPQP, bHPAbsent);
                    strPost4x4Stage1Split(p0 + 48 + j, p1 + j, 0, iHPQP, bHPAbsent);
                    strPost4x4Stage1(p1 + j, 0, iHPQP, bHPAbsent);
                }
            }
        }
        
        if(pSC->WMII.cPostProcStrength > 0 && (!topORleft))
            postProcBlock(pSC->pPostProcInfo, p0, p1, mbX, i, qp[i]); // destairing and first stage deblocking
    }

    //================================================================
    // 420_UV
    for (i = 0; i < (YUV_420 == cfColorFormat? 2U : 0U) && tScale < 16; ++i)
    {
        PixelI* const p0 = pSC->p0MBbuffer[1 + i];//(0 == i ? pSC->pU0 : pSC->pV0);
        PixelI* const p1 = pSC->p1MBbuffer[1 + i];//(0 == i ? pSC->pU1 : pSC->pV1);

        Int iHPQP = 255;
        if (!bHPAbsent)
            iHPQP = pSC->pTile[pSC->cTileColumn].pQuantizerHP[i][pSC->MBInfo.iQIndexHP].iQP;

        //========================================
        // second level inverse transform (420_UV)
        if (!bottomORright)
        {
            if (!pSC->m_param.bScaledArith) {
                JxrTransformMathApplyDct2x2Down(p1, p1 + 32, p1 + 16, p1 + 48);
            }
            else {
                JxrInverseTransformMathApplyScaledDct2x2Down(p1, p1 + 32, p1 + 16, p1 + 48);
            }
        }
        
        //========================================
        // second level inverse overlap (420_UV)
        if (OL_TWO == olOverlap)
        {
            if (leftORright && !topORbottom)
            {
                j = (left ? 0 : -32);
                JxrInverseTransformMathApplyPost2(p0 + j + 16, p1 + j);
            }

            if (!leftORright)
            {
                if (topORbottom)
                {
                    p = (top ? p1 : p0 + 16);
                    JxrInverseTransformMathApplyPost2(p - 32, p);
                    p = NULL;
                }
                else{
                    JxrInverseTransformMathApplyPost2x2(p0 - 16, p0 + 16, p1 - 32, p1);
                }
            }
        }

        //========================================
        // first level inverse transform (420_UV)
        if(tScale >= 4) // bypass first level transform for 4:1 and smaller thumbnail
            continue;

        if (!top)
        {
            for (j = (left ? 16 : -16); j < (right ? 16 : 48); j += 32)
            {
                strIDCT4x4Stage1(p0 + j);
            }
        }

        if (!bottom)
        {
            for (j = (left ? 0 : -32); j < (right ? 0 : 32); j += 32)
            {
                strIDCT4x4Stage1(p1 + j);
            }
        }

        //========================================
        // first level inverse overlap (420_UV)
        if (OL_NONE != olOverlap)
        {
            if(!left && !top)
            {
                if (bottom)
                {
                    for (j = -48; j < (right ? -16 : 16); j += 32)
                    {
                        p = p0 + j;
                        JxrInverseTransformMathApplyPost4(p + 15, p + 14, p + 42, p + 43);
                        JxrInverseTransformMathApplyPost4(p + 13, p + 12, p + 40, p + 41);
                        p = NULL;
                    }
                }
                else
                {
                    for (j = -48; j < (right ? -16 : 16); j += 32)
                    {
                        strPost4x4Stage1Split(p0 + j, p1 - 16 + j, 32, iHPQP, bHPAbsent);
                    }
                }

                if (right)
                {
                    if (!bottom)
                    {
                        JxrInverseTransformMathApplyPost4(p0 - 2 , p0 - 4 , p1 - 28, p1 - 26);
                        JxrInverseTransformMathApplyPost4(p0 - 1 , p0 - 3 , p1 - 27, p1 - 25);
                    }

                    JxrInverseTransformMathApplyPost4(p0 - 18, p0 - 20, p0 - 12, p0 - 10);
                    JxrInverseTransformMathApplyPost4(p0 - 17, p0 - 19, p0 - 11, p0 -  9);
                }
                else
                {
                    strPost4x4Stage1(p0 - 32, 32, iHPQP, bHPAbsent);
                }

                strPost4x4Stage1(p0 - 64, 32, iHPQP, bHPAbsent);
            }
            else if (top)
            {
                for (j = (left ? 0: -64); j < (right ? -32: 0); j += 32)
                {
                    p = p1 + j + 4;
                    JxrInverseTransformMathApplyPost4(p + 1, p + 0, p + 28, p + 29);
                    JxrInverseTransformMathApplyPost4(p + 3, p + 2, p + 30, p + 31);
                    p = NULL;
                }
            }
            else if (left)
            {
                if (!bottom)
                {
                    JxrInverseTransformMathApplyPost4(p0 + 26, p0 + 24, p1 + 0, p1 + 2);
                    JxrInverseTransformMathApplyPost4(p0 + 27, p0 + 25, p1 + 1, p1 + 3);
                }

                JxrInverseTransformMathApplyPost4(p0 + 10, p0 + 8, p0 + 16, p0 + 18);
                JxrInverseTransformMathApplyPost4(p0 + 11, p0 + 9, p0 + 17, p0 + 19);
            }
        }
    }

    //================================================================
    // 422_UV
    for (i = 0; i < (YUV_422 == cfColorFormat? 2U : 0U) && tScale < 16; ++i)
    {
        PixelI* const p0 = pSC->p0MBbuffer[1 + i];//(0 == i ? pSC->pU0 : pSC->pV0);
        PixelI* const p1 = pSC->p1MBbuffer[1 + i];//(0 == i ? pSC->pU1 : pSC->pV1);

        Int iHPQP = 255;
        if (!bHPAbsent)
            iHPQP = pSC->pTile[pSC->cTileColumn].pQuantizerHP[i][pSC->MBInfo.iQIndexHP].iQP;

        //========================================
        // second level inverse transform (422_UV)
        if ((!bottomORright) && pSC->m_Dparam->cThumbnailScale < 16)
        {
            // 1D lossless HT
            p1[0]  -= ((p1[32] + 1) >> 1);
            p1[32] += p1[0];

            if (!pSC->m_param.bScaledArith) {
                JxrTransformMathApplyDct2x2Down(p1 +  0, p1 + 64, p1 + 16, p1 +  80);
                JxrTransformMathApplyDct2x2Down(p1 + 32, p1 + 96, p1 + 48, p1 + 112);
            }
            else {
                JxrInverseTransformMathApplyScaledDct2x2Down(p1 +  0, p1 + 64, p1 + 16, p1 +  80);
                JxrInverseTransformMathApplyScaledDct2x2Down(p1 + 32, p1 + 96, p1 + 48, p1 + 112);
            }
        }
        
        //========================================
        // second level inverse overlap (422_UV)
        if (OL_TWO == olOverlap)
        {
            if (!bottom)
            {
                if (leftORright)
                {
                    if (!top)
                    {
                        j = (left ? 0 : -64);
                        JxrInverseTransformMathApplyPost2(p0 + 48 + j, p1 + j);
                    }

                    j = (left ? 16 : -48);
                    JxrInverseTransformMathApplyPost2(p1 + j, p1 + j + 16);
                }
                else
                {
                    if (top)
                    {
                        JxrInverseTransformMathApplyPost2(p1 - 64, p1);
                    }
                    else
                    {
                        JxrInverseTransformMathApplyPost2x2(p0 - 16, p0 + 48, p1 - 64, p1);
                    }

                    JxrInverseTransformMathApplyPost2x2(p1 - 48, p1 + 16, p1 - 32, p1 + 32);
                }
            }
            else if (!leftORright)
            {
                JxrInverseTransformMathApplyPost2(p0 - 16, p0 + 48);
            }
        }

        //========================================
        // first level inverse transform (422_UV)
        if(tScale >= 4) // bypass first level transform for 4:1 and smaller thumbnail
            continue;

        if (!top)
        {
            for (j = (left ? 48 : -16); j < (right ? 48 : 112); j += 64)
            {
                strIDCT4x4Stage1(p0 + j);
            }
        }

        if (!bottom)
        {
            for (j = (left ? 0 : -64); j < (right ? 0 : 64); j += 64)
            {
                strIDCT4x4Stage1(p1 + j + 0);
                strIDCT4x4Stage1(p1 + j + 16);
                strIDCT4x4Stage1(p1 + j + 32);
            }
        }
        
        //========================================
        // first level inverse overlap (422_UV)
        if (OL_NONE != olOverlap)
        {
            if (!top)
            {
                if (leftORright)
                {
                    j = (left ? 32 + 10 : -32 + 14);

                    p = p0 + j;
                    JxrInverseTransformMathApplyPost4(p + 0, p - 2, p + 6, p + 8);
                    JxrInverseTransformMathApplyPost4(p + 1, p - 1, p + 7, p + 9);

                    p = NULL;
                }

                for (j = (left ? 0 : -128); j < (right ? -64 : 0); j += 64)
                {
                    strPost4x4Stage1(p0 + j + 32, 0, iHPQP, bHPAbsent);
                }
            }

            if (!bottom)
            {
                if (leftORright)
                {
                    j = (left ? 0 + 10 : -64 + 14);

                    p = p1 + j;
                    JxrInverseTransformMathApplyPost4(p + 0, p - 2, p + 6, p + 8);
                    JxrInverseTransformMathApplyPost4(p + 1, p - 1, p + 7, p + 9);

                    p += 16;
                    JxrInverseTransformMathApplyPost4(p + 0, p - 2, p + 6, p + 8);
                    JxrInverseTransformMathApplyPost4(p + 1, p - 1, p + 7, p + 9);

                    p = NULL;
                }

                for (j = (left ? 0 : -128); j < (right ? -64 : 0); j += 64)
                {
                    strPost4x4Stage1(p1 + j +  0, 0, iHPQP, bHPAbsent);
                    strPost4x4Stage1(p1 + j + 16, 0, iHPQP, bHPAbsent);
                }
            }

            if (topORbottom)
            {
                p = (top ? p1 + 5 : p0 + 48 + 13);
                for (j = (left ? 0 : -128); j < (right ? -64 : 0); j += 64)
                {
                    JxrInverseTransformMathApplyPost4(p + j + 0, p + j - 1, p + j + 59, p + j + 60);
                    JxrInverseTransformMathApplyPost4(p + j + 2, p + j + 1, p + j + 61, p + j + 62);
                }
                p = NULL;
            }
            else
            {
                if (leftORright)
                {
                    j = (left ? 0 + 0 : -64 + 4);
                    JxrInverseTransformMathApplyPost4(p0 + j + 48 + 10 + 0, p0 + j + 48 + 10 - 2, p1 + j + 0, p1 + j + 2);
                    JxrInverseTransformMathApplyPost4(p0 + j + 48 + 10 + 1, p0 + j + 48 + 10 - 1, p1 + j + 1, p1 + j + 3);
                }

                for (j = (left ? 0 : -128); j < (right ? -64 : 0); j += 64)
                {
                    strPost4x4Stage1Split(p0 + j + 48, p1 + j + 0, 0, iHPQP, bHPAbsent);
                }
            }
        }
    }    

    return ICERR_OK;
}

Int  invTransformMacroblock_alteredOperators_hard(CWMImageStrCodec * pSC)
{
    const OVERLAP olOverlap = pSC->WMISCP.olOverlap;
    const COLORFORMAT cfColorFormat = pSC->m_param.cfColorFormat;
    // const BITDEPTH_BITS bdBitDepth = pSC->WMII.bdBitDepth;
    const Bool left = (pSC->cColumn == 0), right = (pSC->cColumn == pSC->cmbWidth);
    const Bool top = (pSC->cRow == 0), bottom = (pSC->cRow == pSC->cmbHeight);
    const Bool topORbottom = (top || bottom), leftORright = (left || right);
    const Bool topORleft = (top || left), bottomORright = (bottom || right);
    Bool leftAdjacentColumn = (pSC->cColumn == 1), rightAdjacentColumn = (pSC->cColumn == pSC->cmbWidth - 1);
    // Bool topAdjacentRow =  (pSC->cRow == 1), bottomAdjacentRow = (pSC->cRow == pSC->cmbHeight - 1);
    const size_t mbWidth = pSC->cmbWidth;
    PixelI * p = NULL;// * pt = NULL;
    size_t i;
    const size_t iChannels = (cfColorFormat == YUV_420 || cfColorFormat == YUV_422) ? 1 : pSC->m_param.cNumChannels;
    const size_t tScale = pSC->m_Dparam->cThumbnailScale;
    Int j = 0;

    Int qp[MAX_CHANNELS], dcqp[MAX_CHANNELS], iStrength = (1 << pSC->WMII.cPostProcStrength);
    // ERR_CODE result = ICERR_OK;

    if (pSC->WMISCP.bUseHardTileBoundaries) {
        //Add tile location information
        if (pSC->cColumn == 0) {
            pSC->bVertTileBoundary = FALSE;
            pSC->tileY = 0;
        }
        pSC->bOneMBLeftVertTB = pSC->bOneMBRightVertTB = FALSE;
        if(pSC->tileY > 0 && pSC->tileY <= pSC->WMISCP.cNumOfSliceMinus1H && (pSC->cColumn - 1) == pSC->WMISCP.uiTileY[pSC->tileY])
            pSC->bOneMBRightVertTB = TRUE;
        if(pSC->tileY < pSC->WMISCP.cNumOfSliceMinus1H && pSC->cColumn == pSC->WMISCP.uiTileY[pSC->tileY + 1]) {
            pSC->bVertTileBoundary = TRUE;
            pSC->tileY++;
        }
        else 
            pSC->bVertTileBoundary = FALSE;
        if(pSC->tileY < pSC->WMISCP.cNumOfSliceMinus1H && (pSC->cColumn + 1) == pSC->WMISCP.uiTileY[pSC->tileY + 1])
            pSC->bOneMBLeftVertTB = TRUE;

        if (pSC->cRow == 0) {
            pSC->bHoriTileBoundary = FALSE;
            pSC->tileX = 0;
        }
        else if(pSC->mbY != pSC->cRow && pSC->tileX < pSC->WMISCP.cNumOfSliceMinus1V && pSC->cRow == pSC->WMISCP.uiTileX[pSC->tileX + 1]) {
            pSC->bHoriTileBoundary = TRUE;
            pSC->tileX++;
        }
        else if(pSC->mbY != pSC->cRow)
            pSC->bHoriTileBoundary = FALSE;
    }
    else {
        pSC->bVertTileBoundary = FALSE;
        pSC->bHoriTileBoundary = FALSE;
        pSC->bOneMBLeftVertTB = FALSE;
        pSC->bOneMBRightVertTB = FALSE;
    }
    pSC->mbX = pSC->cColumn, pSC->mbY = pSC->cRow;

    if(pSC->WMII.cPostProcStrength > 0){
        // threshold for post processing
        for(i = 0; i < iChannels; i ++){
            qp[i] = pSC->pTile[pSC->cTileColumn].pQuantizerLP[i][pSC->MBInfo.iQIndexLP].iQP * iStrength * (olOverlap == OL_NONE ? 2 : 1);
            dcqp[i] = pSC->pTile[pSC->cTileColumn].pQuantizerDC[i][0].iQP * iStrength;
        }

        if(left) // a new MB row
            slideOneMBRow(pSC->pPostProcInfo, pSC->m_param.cNumChannels, mbWidth, top, bottom);  // previous current row becomes previous row
    }

    //================================================================
    // 400_Y, 444_YUV
    for (i = 0; i < iChannels && tScale < 16; ++i)
    {
        PixelI* const p0 = pSC->p0MBbuffer[i];
        PixelI* const p1 = pSC->p1MBbuffer[i];

        //================================
        // second level inverse transform
        if (!bottomORright)
        {
            if(pSC->WMII.cPostProcStrength > 0)
                updatePostProcInfo(pSC->pPostProcInfo, p1, pSC->mbX, i); // update postproc info before IDCT

            strIDCT4x4Stage2(p1);
            if (pSC->m_param.bScaledArith) {
                strNormalizeDec(p1, (i != 0));
            }
        }

        //================================
        // second level inverse overlap
        if (OL_TWO == olOverlap)
        {
            /* Corner operations */
            if ((top || pSC->bHoriTileBoundary) && (left || pSC->bVertTileBoundary))
                JxrInverseTransformMathApplyAlternatePost4(p1 + 0, p1 + 64, p1 + 0 + 16, p1 + 64 + 16);
            if ((top || pSC->bHoriTileBoundary) && (right || pSC->bVertTileBoundary))
                JxrInverseTransformMathApplyAlternatePost4(p1 - 128, p1 - 64, p1 - 128 + 16, p1 - 64 + 16);
            if ((bottom || pSC->bHoriTileBoundary) && (left || pSC->bVertTileBoundary))
                JxrInverseTransformMathApplyAlternatePost4(p0 + 32, p0 + 96, p0 + 32 + 16, p0 + 96 + 16);
            if ((bottom || pSC->bHoriTileBoundary) && (right || pSC->bVertTileBoundary))
                JxrInverseTransformMathApplyAlternatePost4(p0 - 96, p0 - 32, p0 - 96 + 16, p0 - 32 + 16);
            if ((leftORright || pSC->bVertTileBoundary) && (!topORbottom  && !pSC->bHoriTileBoundary))
            {
                if (left || pSC->bVertTileBoundary) {
                    j = 0;
                    JxrInverseTransformMathApplyAlternatePost4(p0 + j + 32, p0 + j +  48, p1 + j +  0, p1 + j + 16);
                    JxrInverseTransformMathApplyAlternatePost4(p0 + j + 96, p0 + j + 112, p1 + j + 64, p1 + j + 80);
                }
                if (right || pSC->bVertTileBoundary) {
                    j = -128;
                    JxrInverseTransformMathApplyAlternatePost4(p0 + j + 32, p0 + j +  48, p1 + j +  0, p1 + j + 16);
                    JxrInverseTransformMathApplyAlternatePost4(p0 + j + 96, p0 + j + 112, p1 + j + 64, p1 + j + 80);
                }
            }

            if (!leftORright)
            {
                if ((topORbottom || pSC->bHoriTileBoundary) && !pSC->bVertTileBoundary)
                {
                    if (top || pSC->bHoriTileBoundary) {
                        p = p1;
                        JxrInverseTransformMathApplyAlternatePost4(p - 128, p - 64, p +  0, p + 64);
                        JxrInverseTransformMathApplyAlternatePost4(p - 112, p - 48, p + 16, p + 80);
                        p = NULL;
                    }
                    if (bottom || pSC->bHoriTileBoundary) {
                        p = p0 + 32;
                        JxrInverseTransformMathApplyAlternatePost4(p - 128, p - 64, p +  0, p + 64);
                        JxrInverseTransformMathApplyAlternatePost4(p - 112, p - 48, p + 16, p + 80);
                        p = NULL;
                    }
                }
                
                if (!topORbottom && !pSC->bHoriTileBoundary && !pSC->bVertTileBoundary)
                    strPost4x4Stage2Split_alternate(p0, p1);
            }
        }

        if(pSC->WMII.cPostProcStrength > 0)
            postProcMB(pSC->pPostProcInfo, p0, p1, pSC->mbX, i, dcqp[i]); // second stage deblocking

        //================================
        // first level inverse transform
        if(tScale >= 4) // bypass first level transform for 4:1 and smaller thumbnail
            continue;

        if (!top)
        {
            for (j = (left ? 32 : -96); j < (right ? 32 : 160); j += 64)
            {
                strIDCT4x4Stage1(p0 + j +  0);
                strIDCT4x4Stage1(p0 + j + 16);
            }
        }

        if (!bottom)
        {
            for (j = (left ? 0 : -128); j < (right ? 0 : 128); j += 64)
            {
//                if(tScale == 2  && bdBitDepth != BD_1){
//                    MIPgen(p1 + j + 0);
//                    MIPgen(p1 + j + 16);
//                }
                strIDCT4x4Stage1(p1 + j +  0);
                strIDCT4x4Stage1(p1 + j + 16);
            }
        }

        //================================
        // first level inverse overlap
        if (OL_NONE != olOverlap)
        {
            if (leftORright || pSC->bVertTileBoundary)
            {
                /* Corner operations */
                if ((top || pSC->bHoriTileBoundary) && (left || pSC->bVertTileBoundary))
                    JxrInverseTransformMathApplyAlternatePost4(p1 + 0, p1 + 1, p1 + 2, p1 + 3);
                if ((top || pSC->bHoriTileBoundary) && (right || pSC->bVertTileBoundary))
                    JxrInverseTransformMathApplyAlternatePost4(p1 - 59, p1 - 60, p1 - 57, p1 - 58);
                if ((bottom || pSC->bHoriTileBoundary) && (left || pSC->bVertTileBoundary))
                    JxrInverseTransformMathApplyAlternatePost4(p0 + 48 + 10, p0 + 48 + 11, p0 + 48 + 8, p0 + 48 + 9);
                if ((bottom || pSC->bHoriTileBoundary) && (right || pSC->bVertTileBoundary))
                    JxrInverseTransformMathApplyAlternatePost4(p0 - 1, p0 - 2, p0 - 3, p0 - 4);
                if (left || pSC->bVertTileBoundary) {
                    j = 0 + 10;
                    if (!top)
                    {
                        p = p0 + 16 + j;
                        JxrInverseTransformMathApplyAlternatePost4(p +  0, p -  2, p +  6, p +  8);
                        JxrInverseTransformMathApplyAlternatePost4(p +  1, p -  1, p +  7, p +  9);
                        JxrInverseTransformMathApplyAlternatePost4(p + 16, p + 14, p + 22, p + 24);
                        JxrInverseTransformMathApplyAlternatePost4(p + 17, p + 15, p + 23, p + 25);
                        p = NULL;
                    }
                    if (!bottom)
                    {
                        p = p1 + j;
                        JxrInverseTransformMathApplyAlternatePost4(p + 0, p - 2, p + 6, p + 8);
                        JxrInverseTransformMathApplyAlternatePost4(p + 1, p - 1, p + 7, p + 9);
                        p = NULL;
                    }
                    if (!topORbottom && !pSC->bHoriTileBoundary)
                    {
                        JxrInverseTransformMathApplyAlternatePost4(p0 + 48 + j + 0, p0 + 48 + j - 2, p1 - 10 + j, p1 - 8 + j);
                        JxrInverseTransformMathApplyAlternatePost4(p0 + 48 + j + 1, p0 + 48 + j - 1, p1 -  9 + j, p1 - 7 + j);
                    }
                }
                if (right || pSC->bVertTileBoundary) {
                    j = -64 + 14;
                    if (!top)
                    {
                        p = p0 + 16 + j;
                        JxrInverseTransformMathApplyAlternatePost4(p +  0, p -  2, p +  6, p +  8);
                        JxrInverseTransformMathApplyAlternatePost4(p +  1, p -  1, p +  7, p +  9);
                        JxrInverseTransformMathApplyAlternatePost4(p + 16, p + 14, p + 22, p + 24);
                        JxrInverseTransformMathApplyAlternatePost4(p + 17, p + 15, p + 23, p + 25);
                        p = NULL;
                    }
                    if (!bottom)
                    {
                        p = p1 + j;
                        JxrInverseTransformMathApplyAlternatePost4(p + 0, p - 2, p + 6, p + 8);
                        JxrInverseTransformMathApplyAlternatePost4(p + 1, p - 1, p + 7, p + 9);
                        p = NULL;
                    }
                    if (!topORbottom && !pSC->bHoriTileBoundary)
                    {
                        JxrInverseTransformMathApplyAlternatePost4(p0 + 48 + j + 0, p0 + 48 + j - 2, p1 - 10 + j, p1 - 8 + j);
                        JxrInverseTransformMathApplyAlternatePost4(p0 + 48 + j + 1, p0 + 48 + j - 1, p1 -  9 + j, p1 - 7 + j);
                    }
                }
            }

            if (top || pSC->bHoriTileBoundary)
            {
                for (j = (left ? 0 : -192); j < (right ? -64 : 64); j += 64)
                {
                    if (!pSC->bVertTileBoundary || j != -64) {
                        p = p1 + j;
                        JxrInverseTransformMathApplyAlternatePost4(p + 5, p + 4, p + 64, p + 65);
                        JxrInverseTransformMathApplyAlternatePost4(p + 7, p + 6, p + 66, p + 67);
                        p = NULL;

                        strPost4x4Stage1_alternate(p1 + j, 0);
                    }
                }
            }

            if (bottom || pSC->bHoriTileBoundary)
            {
                for (j = (left ? 0 : -192); j < (right ? -64 : 64); j += 64)
                {
                    if (!pSC->bVertTileBoundary || j != -64) {
                        strPost4x4Stage1_alternate(p0 + 16 + j, 0);
                        strPost4x4Stage1_alternate(p0 + 32 + j, 0);

                        p = p0 + 48 + j;
                        JxrInverseTransformMathApplyAlternatePost4(p + 15, p + 14, p + 74, p + 75);
                        JxrInverseTransformMathApplyAlternatePost4(p + 13, p + 12, p + 72, p + 73);
                        p = NULL;
                    }
                }
            }

            if (!top && !bottom && !pSC->bHoriTileBoundary)
            {
                for (j = (left ? 0 : -192); j < (right ? -64 : 64); j += 64)
                {
                    if (!pSC->bVertTileBoundary || j != -64) {
                        strPost4x4Stage1_alternate(p0 + 16 + j, 0);
                        strPost4x4Stage1_alternate(p0 + 32 + j, 0);
                        strPost4x4Stage1Split_alternate(p0 + 48 + j, p1 + j, 0);
                        strPost4x4Stage1_alternate(p1 + j, 0);
                    }
                }
            }
        }
        
        if(pSC->WMII.cPostProcStrength > 0 && (!topORleft))
            postProcBlock(pSC->pPostProcInfo, p0, p1, pSC->mbX, i, qp[i]); // destairing and first stage deblocking
    }

    //================================================================
    // 420_UV
    for (i = 0; i < (YUV_420 == cfColorFormat? 2U : 0U) && tScale < 16; ++i)
    {
        PixelI* const p0 = pSC->p0MBbuffer[1 + i];//(0 == i ? pSC->pU0 : pSC->pV0);
        PixelI* const p1 = pSC->p1MBbuffer[1 + i];//(0 == i ? pSC->pU1 : pSC->pV1);

        //========================================
        // second level inverse transform (420_UV)
        if (!bottomORright)
        {
            if (!pSC->m_param.bScaledArith) {
                JxrTransformMathApplyDct2x2Down(p1, p1 + 32, p1 + 16, p1 + 48);
            }
            else {
                JxrInverseTransformMathApplyScaledDct2x2Down(p1, p1 + 32, p1 + 16, p1 + 48);
            }
        }
        
        //========================================
        // second level inverse overlap (420_UV)
        if (OL_TWO == olOverlap)
        {
            if ((leftAdjacentColumn || pSC->bOneMBRightVertTB) && (top || pSC->bHoriTileBoundary))
                JxrInverseTransformMathSubtractCornerPrediction(p1 - 64 + 0, *(p1 - 64 + 32));
            if ((rightAdjacentColumn || pSC->bOneMBLeftVertTB) && (top || pSC->bHoriTileBoundary))
                pSC->iPredBefore[i][0] = *(p1 + 0);
            if ((right || pSC->bVertTileBoundary) && (top || pSC->bHoriTileBoundary))
                JxrInverseTransformMathSubtractCornerPrediction(p1 - 64 + 32, pSC->iPredBefore[i][0]);
            if ((leftAdjacentColumn || pSC->bOneMBRightVertTB) && (bottom || pSC->bHoriTileBoundary))
                JxrInverseTransformMathSubtractCornerPrediction(p0 - 64 + 16, *(p0 - 64 + 48));
            if ((rightAdjacentColumn || pSC->bOneMBLeftVertTB) && (bottom || pSC->bHoriTileBoundary))
                pSC->iPredBefore[i][1] = *(p0 + 16);
            if ((right || pSC->bVertTileBoundary) && (bottom || pSC->bHoriTileBoundary))
                JxrInverseTransformMathSubtractCornerPrediction(p0 - 64 + 48, pSC->iPredBefore[i][1]);

            if ((leftORright || pSC->bVertTileBoundary) && !topORbottom && !pSC->bHoriTileBoundary)
            {
                if (left || pSC->bVertTileBoundary)
                    JxrInverseTransformMathApplyAlternatePost2(p0 +   0 + 16, p1 +   0);
                if (right || pSC->bVertTileBoundary)
                    JxrInverseTransformMathApplyAlternatePost2(p0 + -32 + 16, p1 + -32);
            }

            if (!leftORright)
            {
                if ((topORbottom || pSC->bHoriTileBoundary) && !pSC->bVertTileBoundary)
                {
                    if (top || pSC->bHoriTileBoundary)
                        JxrInverseTransformMathApplyAlternatePost2(p1 - 32, p1);
                    if (bottom || pSC->bHoriTileBoundary)
                        JxrInverseTransformMathApplyAlternatePost2(p0 + 16 - 32, p0 + 16);
                }
                else if (!topORbottom && !pSC->bHoriTileBoundary && !pSC->bVertTileBoundary) {
                    JxrInverseTransformMathApplyAlternatePost2x2(p0 - 16, p0 + 16, p1 - 32, p1);
                }
            }
            if ((leftAdjacentColumn || pSC->bOneMBRightVertTB) && (top || pSC->bHoriTileBoundary))
                JxrInverseTransformMathAddCornerPrediction(p1 - 64 + 0, *(p1 - 64 + 32));
            if ((rightAdjacentColumn || pSC->bOneMBLeftVertTB) && (top || pSC->bHoriTileBoundary))
                pSC->iPredAfter[i][0] = *(p1 + 0);
            if ((right || pSC->bVertTileBoundary) && (top || pSC->bHoriTileBoundary))
                JxrInverseTransformMathAddCornerPrediction(p1 - 64 + 32, pSC->iPredAfter[i][0]);
            if ((leftAdjacentColumn || pSC->bOneMBRightVertTB) && (bottom || pSC->bHoriTileBoundary))
                JxrInverseTransformMathAddCornerPrediction(p0 - 64 + 16, *(p0 - 64 + 48));
            if ((rightAdjacentColumn || pSC->bOneMBLeftVertTB) && (bottom || pSC->bHoriTileBoundary))
                pSC->iPredAfter[i][1] = *(p0 + 16);
            if ((right || pSC->bVertTileBoundary) && (bottom || pSC->bHoriTileBoundary))
                JxrInverseTransformMathAddCornerPrediction(p0 - 64 + 48, pSC->iPredAfter[i][1]);
        }

        //========================================
        // first level inverse transform (420_UV)
        if(tScale >= 4) // bypass first level transform for 4:1 and smaller thumbnail
            continue;

        if (!top)
        {
            // In order to allow correction operation of corner chroma overlap operators (fixed)
            // processing of left most MB column must be delayed by one MB 
            // Thus left MB not processed until leftAdjacentColumn = 1
            for (j = ((left) ? 48 : ((leftAdjacentColumn || pSC->bOneMBRightVertTB) ? -48 : -16)); j < ((right || pSC->bVertTileBoundary) ? 16 : 48); j += 32)
            {
                strIDCT4x4Stage1(p0 + j);
            }
        }

        if (!bottom)
        {
            // In order to allow correction operation of corner chroma overlap operators (fixed)
            // processing of left most MB column must be delayed by one MB 
            // Thus left MB not processed until leftAdjacentColumn = 1
            for (j = ((left) ? 32 : ((leftAdjacentColumn || pSC->bOneMBRightVertTB) ? -64 : -32)); j < ((right || pSC->bVertTileBoundary) ? 0 : 32); j += 32)
            {
                strIDCT4x4Stage1(p1 + j);
            }
        }

        //========================================
        // first level inverse overlap (420_UV)
        if (OL_NONE != olOverlap)
        {
            /* Corner operations */
            /* Change because the top-left corner ICT will not have happened until leftAdjacentColumn ==1 */
            if ((top || pSC->bHoriTileBoundary) && (leftAdjacentColumn || pSC->bOneMBRightVertTB))
                JxrInverseTransformMathApplyAlternatePost4(p1 - 64 + 0, p1 - 64 + 1, p1 - 64 + 2, p1 - 64 + 3);
            if ((top || pSC->bHoriTileBoundary) && (right || pSC->bVertTileBoundary))
                JxrInverseTransformMathApplyAlternatePost4(p1 - 27, p1 - 28, p1 - 25, p1 - 26);
            /* Change because the bottom-left corner ICT will not have happened until leftAdjacentColumn ==1 */
            if ((bottom || pSC->bHoriTileBoundary) && (leftAdjacentColumn || pSC->bOneMBRightVertTB))
                JxrInverseTransformMathApplyAlternatePost4(p0 - 64 + 16 + 10, p0 - 64 + 16 + 11, p0 - 64 + 16 + 8, p0 - 64 + 16 + 9);
            if ((bottom || pSC->bHoriTileBoundary) && (right || pSC->bVertTileBoundary))
                JxrInverseTransformMathApplyAlternatePost4(p0 - 1, p0 - 2, p0 - 3, p0 - 4);
            if(!left && !top)
            {
                /* Change because the vertical 1-D overlap operations of the left edge pixels cannot be performed until leftAdjacentColumn ==1 */
                if (leftAdjacentColumn || pSC->bOneMBRightVertTB)
                {
                    if (!bottom && !pSC->bHoriTileBoundary)
                    {
                        JxrInverseTransformMathApplyAlternatePost4(p0 - 64 + 26, p0 - 64 + 24, p1 - 64 + 0, p1 - 64 + 2);
                        JxrInverseTransformMathApplyAlternatePost4(p0 - 64 + 27, p0 - 64 + 25, p1 - 64 + 1, p1 - 64 + 3);
                    }

                    JxrInverseTransformMathApplyAlternatePost4(p0 - 64 + 10, p0 - 64 + 8, p0 - 64 + 16, p0 - 64 + 18);
                    JxrInverseTransformMathApplyAlternatePost4(p0 - 64 + 11, p0 - 64 + 9, p0 - 64 + 17, p0 - 64 + 19);
                }
                if (bottom || pSC->bHoriTileBoundary)
                {
                    p = p0 + -48;
                    JxrInverseTransformMathApplyAlternatePost4(p + 15, p + 14, p + 42, p + 43);
                    JxrInverseTransformMathApplyAlternatePost4(p + 13, p + 12, p + 40, p + 41);
                    p = NULL;

                    if (!right && !pSC->bVertTileBoundary)
                    {
                        p = p0 + -16;
                        JxrInverseTransformMathApplyAlternatePost4(p + 15, p + 14, p + 42, p + 43);
                        JxrInverseTransformMathApplyAlternatePost4(p + 13, p + 12, p + 40, p + 41);
                        p = NULL;
                    }
                }
                else
                {
                    strPost4x4Stage1Split_alternate(p0 + -48, p1 - 16 + -48, 32);

                    if (!right && !pSC->bVertTileBoundary)
                        strPost4x4Stage1Split_alternate(p0 + -16, p1 - 16 + -16, 32);
                }

                if (right || pSC->bVertTileBoundary)
                {
                    if (!bottom && !pSC->bHoriTileBoundary)
                    {
                        JxrInverseTransformMathApplyAlternatePost4(p0 - 2 , p0 - 4 , p1 - 28, p1 - 26);
                        JxrInverseTransformMathApplyAlternatePost4(p0 - 1 , p0 - 3 , p1 - 27, p1 - 25);
                    }

                    JxrInverseTransformMathApplyAlternatePost4(p0 - 18, p0 - 20, p0 - 12, p0 - 10);
                    JxrInverseTransformMathApplyAlternatePost4(p0 - 17, p0 - 19, p0 - 11, p0 -  9);
                }
                else
                {
                    strPost4x4Stage1_alternate(p0 - 32, 32);
                }

                strPost4x4Stage1_alternate(p0 - 64, 32);
            }

            if (top || pSC->bHoriTileBoundary)
            {
                if (!left)
                {
                    p = p1 + -64 + 4;
                    JxrInverseTransformMathApplyAlternatePost4(p + 1, p + 0, p + 28, p + 29);
                    JxrInverseTransformMathApplyAlternatePost4(p + 3, p + 2, p + 30, p + 31);
                    p = NULL;
                }

                if (!left && !right && !pSC->bVertTileBoundary)
                {
                    p = p1 + -32 + 4;
                    JxrInverseTransformMathApplyAlternatePost4(p + 1, p + 0, p + 28, p + 29);
                    JxrInverseTransformMathApplyAlternatePost4(p + 3, p + 2, p + 30, p + 31);
                    p = NULL;
                }
            }
        }
    }

    //================================================================
    // 422_UV
    for (i = 0; i < (YUV_422 == cfColorFormat? 2U : 0U) && tScale < 16; ++i)
    {
        PixelI* const p0 = pSC->p0MBbuffer[1 + i];//(0 == i ? pSC->pU0 : pSC->pV0);
        PixelI* const p1 = pSC->p1MBbuffer[1 + i];//(0 == i ? pSC->pU1 : pSC->pV1);

        //========================================
        // second level inverse transform (422_UV)
        if ((!bottomORright) && pSC->m_Dparam->cThumbnailScale < 16)
        {
            // 1D lossless HT
            p1[0]  -= ((p1[32] + 1) >> 1);
            p1[32] += p1[0];

            if (!pSC->m_param.bScaledArith) {
                JxrTransformMathApplyDct2x2Down(p1 +  0, p1 + 64, p1 + 16, p1 +  80);
                JxrTransformMathApplyDct2x2Down(p1 + 32, p1 + 96, p1 + 48, p1 + 112);
            }
            else {
                JxrInverseTransformMathApplyScaledDct2x2Down(p1 +  0, p1 + 64, p1 + 16, p1 +  80);
                JxrInverseTransformMathApplyScaledDct2x2Down(p1 + 32, p1 + 96, p1 + 48, p1 + 112);
            }
        }
        
        //========================================
        // second level inverse overlap (422_UV)
        if (OL_TWO == olOverlap)
        {
            if ((leftAdjacentColumn || pSC->bOneMBRightVertTB) && (top || pSC->bHoriTileBoundary))
                JxrInverseTransformMathSubtractCornerPrediction(p1 - 128 + 0, *(p1 - 128 + 64));

            if ((rightAdjacentColumn || pSC->bOneMBLeftVertTB) && (top || pSC->bHoriTileBoundary))
                pSC->iPredBefore[i][0] = *(p1 + 0);
            if ((right || pSC->bVertTileBoundary) && (top || pSC->bHoriTileBoundary))
                JxrInverseTransformMathSubtractCornerPrediction(p1 - 128 + 64, pSC->iPredBefore[i][0]);

            if ((leftAdjacentColumn || pSC->bOneMBRightVertTB) && (bottom || pSC->bHoriTileBoundary))
                JxrInverseTransformMathSubtractCornerPrediction(p0 - 128 + 48, *(p0 - 128 + 112));

            if ((rightAdjacentColumn || pSC->bOneMBLeftVertTB) && (bottom || pSC->bHoriTileBoundary))
                pSC->iPredBefore[i][1] = *(p0 + 48);
            if ((right || pSC->bVertTileBoundary) && (bottom || pSC->bHoriTileBoundary))
                JxrInverseTransformMathSubtractCornerPrediction(p0 - 128 + 112, pSC->iPredBefore[i][1]);

            if (!bottom)
            {
                if (leftORright || pSC->bVertTileBoundary)
                {
                    if (!top && !pSC->bHoriTileBoundary)
                    {
                        if (left || pSC->bVertTileBoundary)
                            JxrInverseTransformMathApplyAlternatePost2(p0 + 48 + 0, p1 + 0);

                        if (right || pSC->bVertTileBoundary)
                            JxrInverseTransformMathApplyAlternatePost2(p0 + 48 + -64, p1 + -64);
                    }

                    if (left || pSC->bVertTileBoundary)
                        JxrInverseTransformMathApplyAlternatePost2(p1 + 16, p1 + 16 + 16);

                    if (right || pSC->bVertTileBoundary)
                        JxrInverseTransformMathApplyAlternatePost2(p1 + -48, p1 + -48 + 16);
                }

                if (!leftORright && !pSC->bVertTileBoundary)
                {
                    if (top || pSC->bHoriTileBoundary)
                        JxrInverseTransformMathApplyAlternatePost2(p1 - 64, p1);
                    else
                        JxrInverseTransformMathApplyAlternatePost2x2(p0 - 16, p0 + 48, p1 - 64, p1);

                    JxrInverseTransformMathApplyAlternatePost2x2(p1 - 48, p1 + 16, p1 - 32, p1 + 32);
                }
            }
            
            if ((bottom || pSC->bHoriTileBoundary) && (!leftORright && !pSC->bVertTileBoundary))
                JxrInverseTransformMathApplyAlternatePost2(p0 - 16, p0 + 48);

            if ((leftAdjacentColumn || pSC->bOneMBRightVertTB) && (top || pSC->bHoriTileBoundary))
                JxrInverseTransformMathAddCornerPrediction(p1 - 128 + 0, *(p1 - 128 + 64));

            if ((rightAdjacentColumn || pSC->bOneMBLeftVertTB) && (top || pSC->bHoriTileBoundary))
                pSC->iPredAfter[i][0] = *(p1 + 0);
            if ((right || pSC->bVertTileBoundary) && (top || pSC->bHoriTileBoundary))
                JxrInverseTransformMathAddCornerPrediction(p1 - 128 + 64, pSC->iPredAfter[i][0]);

            if ((leftAdjacentColumn || pSC->bOneMBRightVertTB) && (bottom || pSC->bHoriTileBoundary))
                JxrInverseTransformMathAddCornerPrediction(p0 - 128 + 48, *(p0 - 128 + 112));

            if ((rightAdjacentColumn || pSC->bOneMBLeftVertTB) && (bottom || pSC->bHoriTileBoundary))
                pSC->iPredAfter[i][1] = *(p0 + 48);
            if ((right || pSC->bVertTileBoundary) && (bottom || pSC->bHoriTileBoundary))
                JxrInverseTransformMathAddCornerPrediction(p0 - 128 + 112, pSC->iPredAfter[i][1]);
        }

        //========================================
        // first level inverse transform (422_UV)
        if(tScale >= 4) // bypass first level transform for 4:1 and smaller thumbnail
            continue;

        if (!top)
        {
            // Need to delay processing of left column until leftAdjacentColumn = 1 for corner overlap operators
            // Since 422 has no vertical downsampling, no top MB delay of processing is necessary
            for (j = (left ? 112 : ((leftAdjacentColumn || pSC->bOneMBRightVertTB) ? -80 : -16)); j < ((right || pSC->bVertTileBoundary) ? 48 : 112); j += 64)
            {
                strIDCT4x4Stage1(p0 + j);
            }
        }

        if (!bottom)
        {
            // Need to delay processing of left column until leftAdjacentColumn = 1 for corner overlap operators
            // Since 422 has no vertical downsampling, no top MB delay of processing is necessary
            for (j = (left ? 64 : ((leftAdjacentColumn || pSC->bOneMBRightVertTB) ? -128 : -64)); j < ((right || pSC->bVertTileBoundary) ? 0 : 64); j += 64)
            {
                strIDCT4x4Stage1(p1 + j + 0);
                strIDCT4x4Stage1(p1 + j + 16);
                strIDCT4x4Stage1(p1 + j + 32);
            }
        }
        
        //========================================
        // first level inverse overlap (422_UV)
        if (OL_NONE != olOverlap)
        {
            /* Corner operations */
            if ((top || pSC->bHoriTileBoundary) && (leftAdjacentColumn || pSC->bOneMBRightVertTB))
                JxrInverseTransformMathApplyAlternatePost4(p1 - 128 + 0, p1 - 128 + 1, p1 - 128 + 2, p1 - 128 + 3);
            if ((top || pSC->bHoriTileBoundary) && (right || pSC->bVertTileBoundary))
                JxrInverseTransformMathApplyAlternatePost4(p1 - 59, p1 - 60, p1 - 57, p1 - 58);
            if ((bottom || pSC->bHoriTileBoundary) && (leftAdjacentColumn || pSC->bOneMBRightVertTB))
                JxrInverseTransformMathApplyAlternatePost4(p0 - 128 + 48 + 10, p0 - 128 + 48 + 11, p0 - 128 + 48 + 8, p0 - 128 + 48 + 9);
            if ((bottom || pSC->bHoriTileBoundary) && (right || pSC->bVertTileBoundary))
                JxrInverseTransformMathApplyAlternatePost4(p0 - 1, p0 - 2, p0 - 3, p0 - 4);
            if (!top)
            {
                // Need to delay processing of left column until leftAdjacentColumn = 1 for corner overlap operators
                if (leftAdjacentColumn || pSC->bOneMBRightVertTB) {
                    p = p0 + 32 + 10 - 128;
                    JxrInverseTransformMathApplyAlternatePost4(p + 0, p - 2, p + 6, p + 8);
                    JxrInverseTransformMathApplyAlternatePost4(p + 1, p - 1, p + 7, p + 9);
                    p = NULL;
                }

                if (right || pSC->bVertTileBoundary) {
                    p = p0 + -32 + 14;
                    JxrInverseTransformMathApplyAlternatePost4(p + 0, p - 2, p + 6, p + 8);
                    JxrInverseTransformMathApplyAlternatePost4(p + 1, p - 1, p + 7, p + 9);
                    p = NULL;
                }

                for (j = (left ? 0 : -128); j < ((right || pSC->bVertTileBoundary) ? -64 : 0); j += 64)
                    strPost4x4Stage1_alternate(p0 + j + 32, 0);
            }

            if (!bottom)
            {
                // Need to delay processing of left column until leftAdjacentColumn = 1 for corner overlap operators
                if (leftAdjacentColumn || pSC->bOneMBRightVertTB)
                {
                    p = p1 + 0 + 10 - 128;
                    JxrInverseTransformMathApplyAlternatePost4(p + 0, p - 2, p + 6, p + 8);
                    JxrInverseTransformMathApplyAlternatePost4(p + 1, p - 1, p + 7, p + 9);
                    p += 16;
                    JxrInverseTransformMathApplyAlternatePost4(p + 0, p - 2, p + 6, p + 8);
                    JxrInverseTransformMathApplyAlternatePost4(p + 1, p - 1, p + 7, p + 9);
                    p = NULL;
                }

                if (right || pSC->bVertTileBoundary)
                {
                    p = p1 + -64 + 14;
                    JxrInverseTransformMathApplyAlternatePost4(p + 0, p - 2, p + 6, p + 8);
                    JxrInverseTransformMathApplyAlternatePost4(p + 1, p - 1, p + 7, p + 9);
                    p += 16;
                    JxrInverseTransformMathApplyAlternatePost4(p + 0, p - 2, p + 6, p + 8);
                    JxrInverseTransformMathApplyAlternatePost4(p + 1, p - 1, p + 7, p + 9);
                    p = NULL;
                }

                for (j = (left ? 0 : -128); j < ((right || pSC->bVertTileBoundary) ? -64 : 0); j += 64)
                {
                    strPost4x4Stage1_alternate(p1 + j +  0, 0);
                    strPost4x4Stage1_alternate(p1 + j + 16, 0);
                }
            }

            if (topORbottom || pSC->bHoriTileBoundary)
            {
                if (top || pSC->bHoriTileBoundary) {
                    p = p1 + 5;
                    for (j = (left ? 0 : -128); j < ((right || pSC->bVertTileBoundary) ? -64 : 0); j += 64)
                    {
                        JxrInverseTransformMathApplyAlternatePost4(p + j + 0, p + j - 1, p + j + 59, p + j + 60);
                        JxrInverseTransformMathApplyAlternatePost4(p + j + 2, p + j + 1, p + j + 61, p + j + 62);
                    }
                    p = NULL;
                }

                if (bottom || pSC->bHoriTileBoundary) {
                    p = p0 + 48 + 13;
                    for (j = (left ? 0 : -128); j < ((right || pSC->bVertTileBoundary) ? -64 : 0); j += 64)
                    {
                        JxrInverseTransformMathApplyAlternatePost4(p + j + 0, p + j - 1, p + j + 59, p + j + 60);
                        JxrInverseTransformMathApplyAlternatePost4(p + j + 2, p + j + 1, p + j + 61, p + j + 62);
                    }
                    p = NULL;
                }
            }
            else
            {
                // Need to delay processing of left column until leftAdjacentColumn = 1 for corner overlap operators
                if (leftAdjacentColumn || pSC->bOneMBRightVertTB)
                {
                    j = 0 + 0 - 128;
                    JxrInverseTransformMathApplyAlternatePost4(p0 + j + 48 + 10 + 0, p0 + j + 48 + 10 - 2, p1 + j + 0, p1 + j + 2);
                    JxrInverseTransformMathApplyAlternatePost4(p0 + j + 48 + 10 + 1, p0 + j + 48 + 10 - 1, p1 + j + 1, p1 + j + 3);
                }

                if (right || pSC->bVertTileBoundary)
                {
                    j = -64 + 4;
                    JxrInverseTransformMathApplyAlternatePost4(p0 + j + 48 + 10 + 0, p0 + j + 48 + 10 - 2, p1 + j + 0, p1 + j + 2);
                    JxrInverseTransformMathApplyAlternatePost4(p0 + j + 48 + 10 + 1, p0 + j + 48 + 10 - 1, p1 + j + 1, p1 + j + 3);
                }

                for (j = (left ? 0 : -128); j < ((right || pSC->bVertTileBoundary) ? -64 : 0); j += 64)
                    strPost4x4Stage1Split_alternate(p0 + j + 48, p1 + j + 0, 0);
            }
        }
    }    

    return ICERR_OK;
}
