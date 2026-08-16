#include "JxrHardTileCodecStateAdapter.h"

Void JxrHardTileCodecStateAdapterUpdate(
    CWMImageStrCodec* codec,
    const JxrInverseTransformMacroblockGeometry* geometry,
    JxrHardTileBoundaryState* hardTileState,
    JxrInverseTransformBoundaryContext* boundaryContext)
{
    JxrHardTileBoundaryConfiguration configuration;
    JxrHardTileBoundaryState previous;

    configuration.enabled = codec->WMISCP.bUseHardTileBoundaries;
    configuration.verticalSliceCountMinusOne = codec->WMISCP.cNumOfSliceMinus1V;
    configuration.horizontalSliceCountMinusOne = codec->WMISCP.cNumOfSliceMinus1H;
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

    JxrHardTileBoundaryStateCalculate(hardTileState, &previous, &configuration,
        codec->cColumn, codec->cRow);
    JxrInverseTransformBoundaryContextInitialize(boundaryContext, geometry, hardTileState);

    codec->tileX = hardTileState->tileX;
    codec->tileY = hardTileState->tileY;
    codec->mbX = hardTileState->previousMacroblockX;
    codec->mbY = hardTileState->previousMacroblockY;
    codec->bVertTileBoundary = hardTileState->isVerticalBoundary;
    codec->bHoriTileBoundary = hardTileState->isHorizontalBoundary;
    codec->bOneMBLeftVertTB = hardTileState->isOneMacroblockLeftOfVerticalBoundary;
    codec->bOneMBRightVertTB = hardTileState->isOneMacroblockRightOfVerticalBoundary;
}
