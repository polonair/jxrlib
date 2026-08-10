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

#include "strcodec.h"
#include "decode.h"
#include "JxrEntropyReader.h"
#include "JxrQuantizationIndexReader.h"
#include "JxrAdaptiveHuffman.h"
#include "JxrAdaptiveScan.h"
#include "JxrCoefficientBuffer.h"
#include "JxrEntropyLevelDecoder.h"
#include "JxrDcDecoder.h"
#include "JxrEntropyBlockDecoder.h"
#include "JxrLpDecoder.h"
#include "JxrHpDecoder.h"

#ifdef MEM_TRACE
#define TRACE_MALLOC    1
#define TRACE_NEW       0
#define TRACE_HEAP      0
#include "memtrace.h"
#endif

extern const int dctIndex[3][16];
extern const int blkOffset[16];
extern const int blkOffsetUV[4];

static Int AdaptDecFixed(CAdaptiveHuffman* state)
{
    JxrAdaptiveHuffmanAdapt(state);
    return ICERR_OK;
}

U8 decodeQPIndex(BitIOInfo* input, U8 bitCount)
{
    JxrEntropyBitReader reader;
    JxrEntropyBitReaderInit(&reader, input);
    return JxrQuantizationIndexReaderDecode(&reader, bitCount);
}

/*************************************************************************
    DecodeSecondStageCoeff
*************************************************************************/
Int DecodeMacroblockLowpass(CWMImageStrCodec* pSC, CCodingContext* pContext, Int iMBX, Int iMBYdummy)
{
    JxrDecoderSubbandContext state;
    JxrDecoderSubbandContextInit(&state, pSC, pContext);
    return JxrLpDecoderDecodeSubband(&state, iMBX, iMBYdummy);
}

/*************************************************************************
    8 bit YUV 420 macroblock decode function with 4x4 transform
    Index order is as follows:
    Y:              U:      V:
     0  1  4  5     16 17   20 21
     2  3  6  7     18 19   22 23
     8  9 12 13
    10 11 14 15

    DCAC coefficients stored for 4x4 - offsets (x == no storage)
    Y:
    x x x [0..3]
    x x x [4..7]
    x x x [8..11]
    [16..19] [20..23] [24..27] [28..31,12..15]

    U, V:
    x [0..3]
    [8..11] [4..7,12..15]
*************************************************************************/
Int DecodeMacroblockDC(CWMImageStrCodec* pSC, CCodingContext* pContext, Int iMBX, Int iMBY)
{
    JxrDecoderSubbandContext state;
    JxrDecoderSubbandContextInit(&state, pSC, pContext);
    return JxrDcDecoderDecodeSubband(&state, iMBX, iMBY);
}

/*************************************************************************
    DecodeMacroblockHighpass
*************************************************************************/
Int DecodeMacroblockHighpass(CWMImageStrCodec* pSC, CCodingContext* pContext, Int iMBX, Int iMBY)
{
    JxrDecoderSubbandContext state;
    JxrDecoderSubbandContextInit(&state, pSC, pContext);
    return JxrHpDecoderDecodeSubband(&state, iMBX, iMBY);
}

/*************************************************************************
    Adapt
*************************************************************************/
Int AdaptLowpassDec(CCodingContext * pSC)
{
    Int kk;
    for (kk = 0; kk < CONTEXTX + CTDC; kk++) {
        if (ICERR_OK != AdaptDecFixed (pSC->m_pAHexpt[kk])) {
            goto ErrorExit;
        }
    }
    return ICERR_OK;

ErrorExit:
    return ICERR_ERROR;

}

Int AdaptHighpassDec(CCodingContext * pSC)
{
    Int kk;
    if (ICERR_OK != AdaptDecFixed (pSC->m_pAdaptHuffCBPCY)) {
        goto ErrorExit;
    }
    if (ICERR_OK != AdaptDecFixed (pSC->m_pAdaptHuffCBPCY1)) {
        goto ErrorExit;
    }
    for (kk = 0; kk < CONTEXTX; kk++) {
        if (ICERR_OK != AdaptDecFixed (pSC->m_pAHexpt[kk + CONTEXTX + CTDC])) {
            goto ErrorExit;
        }
    }

    return ICERR_OK;
    
ErrorExit:
    return ICERR_ERROR;
}
