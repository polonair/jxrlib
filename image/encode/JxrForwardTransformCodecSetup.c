#include "JxrForwardTransformCodecSetup.h"

#include "JxrForwardHardTileCodecStateAdapter.h"

Void JxrForwardTransformCodecSetupInitialize(
    JxrForwardTransformCodecSetup* setup,
    CWMImageStrCodec* codec)
{
    JxrForwardHardTileCodecStateAdapterUpdate(codec, &setup->hardTileState);
    JxrForwardTransformMacroblockGeometryInitialize(&setup->geometry,
        codec->WMISCP.olOverlap, codec->m_param.cfColorFormat,
        codec->cColumn, codec->cRow, codec->cmbWidth, codec->cmbHeight,
        codec->m_param.cNumChannels);
    JxrForwardTransformBoundaryContextInitialize(&setup->boundaries,
        &setup->geometry, &setup->hardTileState);
    setup->usesScaledArithmetic = codec->m_param.bScaledArith;
}
