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
#include "encode.h"
#include "JxrForwardHardTileCodecStateAdapter.h"
#include "JxrForwardTransformMacroblockGeometry.h"
#include "JxrForwardTransformBoundaryContext.h"
#include "JxrForwardTransformFullResolutionPlane.h"
#include "JxrForwardTransformChroma420Plane.h"
#include "JxrForwardTransformChroma422Plane.h"

/** Top-level forward transform orchestration. **/

Void transformMacroblock(CWMImageStrCodec * pSC)
{
    JxrForwardTransformMacroblockGeometry geometry;
    JxrForwardHardTileBoundaryState hardTileState;
    JxrForwardTransformBoundaryContext boundaries;
    Int i;

    JxrForwardHardTileCodecStateAdapterUpdate(pSC, &hardTileState);
    JxrForwardTransformMacroblockGeometryInitialize(&geometry,
        pSC->WMISCP.olOverlap, pSC->m_param.cfColorFormat, pSC->cColumn, pSC->cRow,
        pSC->cmbWidth, pSC->cmbHeight, pSC->m_param.cNumChannels);
    JxrForwardTransformBoundaryContextInitialize(&boundaries, &geometry, &hardTileState);

    //================================================================
    // 400_Y, 444_YUV
    for(i = 0; i < (Int)geometry.fullResolutionPlaneCount; ++i)
    {
        JxrForwardTransformFullResolutionPlaneApply(pSC->p0MBbuffer[i], pSC->p1MBbuffer[i],
            i != 0, &geometry, &boundaries, pSC->m_param.bScaledArith);
    }

    //================================================================
    // 420_UV
    for(i = 0; i < (YUV_420 == geometry.colorFormat ? 2 : 0); ++i)
    {
        JxrForwardTransformChroma420PlaneApply(pSC->p0MBbuffer[1 + i], pSC->p1MBbuffer[1 + i],
            pSC->iPredBefore[i], pSC->iPredAfter[i], &geometry, &boundaries,
            pSC->m_param.bScaledArith);
    }

    //================================================================
    // 422_UV
    for(i = 0; i < (YUV_422 == geometry.colorFormat ? 2 : 0); ++i)
    {
        JxrForwardTransformChroma422PlaneApply(pSC->p0MBbuffer[1 + i], pSC->p1MBbuffer[1 + i],
            pSC->iPredBefore[i], pSC->iPredAfter[i], &geometry, &boundaries,
            pSC->m_param.bScaledArith);
    }
}

