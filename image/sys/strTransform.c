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

const Int JxrTransformFirstStageFourButterflyOffsets[16] = {
    0, 4, 8, 12,
    1, 5, 9, 13,
    2, 6, 10, 14,
    3, 7, 11, 15
};

/** need to swap b and c **/
/** rounding behavior: [0 0 0 0] <-> [+ - - -]
    [+ + + +] <-> [+3/4 - - -]
    [- - - -] <-> [- - - -] **/
Void strDCT2x2dn(PixelI *pa, PixelI *pb, PixelI *pc, PixelI *pd)
{
    PixelI a, b, c, d, C, t;
    a = *pa;
    b = *pb;
    C = *pc;
    d = *pd;
  
    a += d;
    b -= C;
    t = ((a - b) >> 1);
    c = t - d;
    d = t - C;
    a -= d;
    b += c;

    *pa = a;
    *pb = b;
    *pc = c;
    *pd = d;
}

Void strDCT2x2up(PixelI *pa, PixelI *pb, PixelI *pc, PixelI *pd)
{
    PixelI a, b, c, d, C, t;
    a = *pa;
    b = *pb;
    C = *pc;
    d = *pd;
  
    a += d;
    b -= C;
    t = ((a - b + 1) >> 1);
    c = t - d;
    d = t - C;
    a -= d;
    b += c;

    *pa = a;
    *pb = b;
    *pc = c;
    *pd = d;
}

Void strTransformSubtractCornerPrediction(PixelI* target, PixelI prediction)
{
    *target -= prediction;
}

Void strTransformAddCornerPrediction(PixelI* target, PixelI prediction)
{
    *target += prediction;
}

Void strTransformApplyFourButterfly(PixelI* buffer, const Int* offsets)
{
    Int group;

    for (group = 0; group < 4; ++group) {
        Int index = group * 4;
        strDCT2x2dn(&buffer[offsets[index]], &buffer[offsets[index + 1]],
            &buffer[offsets[index + 2]], &buffer[offsets[index + 3]]);
    }
}
