#include "strTransform.h"
#include "strcodec.h"
#include "decode.h"
#include "JxrInverseTransformAlternateCodecSetup.h"

Void JxrInverseTransformAlternateCodecSetupInitialize(
    JxrInverseTransformAlternateCodecSetup* setup,
    CWMImageStrCodec* codec)
{
    JxrInverseTransformMacroblockGeometryInitialize(&setup->geometry,
        codec->WMISCP.olOverlap, codec->m_param.cfColorFormat,
        codec->cColumn, codec->cRow, codec->cmbWidth, codec->cmbHeight,
        codec->m_param.cNumChannels, codec->m_Dparam->cThumbnailScale);
    JxrInverseTransformPlanePlanInitialize(&setup->planePlan,
        setup->geometry.colorFormat, setup->geometry.channelCount,
        setup->geometry.thumbnailScale);
    JxrHardTileCodecStateAdapterUpdate(codec, &setup->geometry,
        &setup->hardTileState, &setup->boundaryContext);
    JxrInversePostProcessParametersInitialize(&setup->postProcessParameters,
        codec->WMII.cPostProcStrength, setup->geometry.overlap,
        setup->geometry.channelCount, codec->pTile[codec->cTileColumn].pQuantizerLP,
        codec->pTile[codec->cTileColumn].pQuantizerDC, codec->MBInfo.iQIndexLP);
}
