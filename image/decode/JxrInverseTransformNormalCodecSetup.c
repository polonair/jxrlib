#include "JxrInverseTransformNormalCodecSetup.h"

Void JxrInverseTransformNormalCodecSetupInitialize(
    JxrInverseTransformNormalCodecSetup* setup,
    const CWMImageStrCodec* codec)
{
    JxrInverseTransformMacroblockGeometryInitialize(&setup->geometry,
        codec->WMISCP.olOverlap, codec->m_param.cfColorFormat,
        codec->cColumn, codec->cRow, codec->cmbWidth, codec->cmbHeight,
        codec->m_param.cNumChannels, codec->m_Dparam->cThumbnailScale);
    JxrInverseTransformPlanePlanInitialize(&setup->planePlan,
        setup->geometry.colorFormat, setup->geometry.channelCount,
        setup->geometry.thumbnailScale);
    JxrInversePostProcessParametersInitialize(&setup->postProcessParameters,
        codec->WMII.cPostProcStrength, setup->geometry.overlap,
        setup->geometry.channelCount, codec->pTile[codec->cTileColumn].pQuantizerLP,
        codec->pTile[codec->cTileColumn].pQuantizerDC, codec->MBInfo.iQIndexLP);
    JxrInverseHighPassParametersInitialize(&setup->highPassParameters,
        codec->WMISCP.sbSubband, codec->m_param.cNumChannels,
        codec->pTile[codec->cTileColumn].pQuantizerHP, codec->MBInfo.iQIndexHP);
}
