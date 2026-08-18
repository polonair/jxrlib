#include "JxrForwardHardTileCodecStateAdapter.h"
#include "JxrForwardHardTileBoundaryState.h"

Void JxrForwardHardTileCodecStateAdapterUpdate(CWMImageStrCodec* codec)
{
    JxrForwardHardTileBoundaryConfiguration configuration;
    JxrForwardHardTileBoundaryState previous;
    JxrForwardHardTileBoundaryState result;

    configuration.enabled = codec->WMISCP.bUseHardTileBoundaries;
    configuration.verticalSliceCountMinusOne = codec->WMISCP.cNumOfSliceMinus1H;
    configuration.horizontalSliceCountMinusOne = codec->WMISCP.cNumOfSliceMinus1V;
    configuration.verticalSliceColumns = codec->WMISCP.uiTileY;
    configuration.horizontalSliceRows = codec->WMISCP.uiTileX;

    previous.tileX = codec->tileX;
    previous.tileY = codec->tileY;
    previous.previousMacroblockX = codec->mbX;
    previous.previousMacroblockY = codec->mbY;
    previous.isVerticalBoundary = codec->bVertTileBoundary;
    previous.isHorizontalBoundary = codec->bHoriTileBoundary;
    previous.isOneMacroblockLeftOfVerticalBoundary = codec->bOneMBLeftVertTB;
    previous.isOneMacroblockRightOfVerticalBoundary = codec->bOneMBRightVertTB;

    JxrForwardHardTileBoundaryStateCalculate(&result, &previous, &configuration,
        codec->cColumn, codec->cRow);

    codec->tileX = result.tileX;
    codec->tileY = result.tileY;
    codec->mbX = result.previousMacroblockX;
    codec->mbY = result.previousMacroblockY;
    codec->bVertTileBoundary = result.isVerticalBoundary;
    codec->bHoriTileBoundary = result.isHorizontalBoundary;
    codec->bOneMBLeftVertTB = result.isOneMacroblockLeftOfVerticalBoundary;
    codec->bOneMBRightVertTB = result.isOneMacroblockRightOfVerticalBoundary;
}
