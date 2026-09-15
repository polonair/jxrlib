#include "JxrTranscodeTileExtractionExecutor.h"

#include "../encode/JxrEncoderLegacyContract.h"
#include "../encode/JxrEncoderPacketStreamAssembler.h"

EXTERN_C Int writeIndexTable(CWMImageStrCodec* codec);

size_t JxrTranscodeTileExtractionExecutorPacketCount(BITSTREAMFORMAT layout,
    SUBBAND subband)
{
    if (layout == SPATIAL || subband == SB_DC_ONLY) return 1;
    if (subband == SB_NO_HIGHPASS) return 2;
    if (subband == SB_NO_FLEXBITS) return 3;
    return 4;
}

Bool JxrTranscodeTileExtractionExecutorContainsTile(U32 tileColumn, U32 tileRow,
    size_t macroblockLeft, size_t macroblockRight,
    size_t macroblockTop, size_t macroblockBottom)
{
    return tileColumn >= macroblockLeft && tileColumn < macroblockRight &&
        tileRow >= macroblockTop && tileRow < macroblockBottom;
}

Int JxrTranscodeTileExtractionExecutorExecute(CWMImageStrCodec* sourceCodec,
    CWMImageStrCodec* destinationCodec, size_t macroblockLeft,
    size_t macroblockRight, size_t macroblockTop, size_t macroblockBottom)
{
    size_t sourcePacketCount;
    size_t destinationPacketCount;
    size_t sourceTileColumnCount;
    size_t tileColumnCount;
    size_t tileRowCount;
    size_t tileCount;
    size_t tileRow;
    size_t tileColumn;
    size_t packet;
    size_t outputIndex = 0;

    if (sourceCodec == NULL || destinationCodec == NULL ||
        sourceCodec->pIndexTable == NULL || sourceCodec->WMISCP.pWStream == NULL ||
        destinationCodec->WMISCP.pWStream == NULL || macroblockLeft > macroblockRight ||
        macroblockTop > macroblockBottom) return ICERR_ERROR;
    sourcePacketCount = JxrTranscodeTileExtractionExecutorPacketCount(
        sourceCodec->WMISCP.bfBitstreamFormat, sourceCodec->WMISCP.sbSubband);
    destinationPacketCount = JxrTranscodeTileExtractionExecutorPacketCount(
        destinationCodec->WMISCP.bfBitstreamFormat, destinationCodec->WMISCP.sbSubband);
    if (destinationPacketCount > sourcePacketCount) return ICERR_ERROR;
    tileColumnCount = destinationCodec->WMISCP.cNumOfSliceMinus1V + 1;
    sourceTileColumnCount = sourceCodec->WMISCP.cNumOfSliceMinus1V + 1;
    tileRowCount = destinationCodec->WMISCP.cNumOfSliceMinus1H + 1;
    if (tileColumnCount == 0 || tileRowCount > ((size_t)-1) / tileColumnCount)
        return ICERR_ERROR;
    tileCount = tileRowCount * tileColumnCount;
    if (destinationPacketCount > 0 && tileCount > ((size_t)-1) / destinationPacketCount)
        return ICERR_ERROR;
    destinationCodec->pIndexTable = (size_t*)malloc(sizeof(size_t) * tileCount *
        destinationPacketCount);
    if (destinationCodec->pIndexTable == NULL) return ICERR_ERROR;
    destinationCodec->cNumBitIO = destinationPacketCount * tileColumnCount;

    for (tileRow = 0; tileRow < sourceCodec->WMISCP.cNumOfSliceMinus1H + 1; ++tileRow)
        for (tileColumn = 0; tileColumn < sourceCodec->WMISCP.cNumOfSliceMinus1V + 1;
            ++tileColumn)
            if (JxrTranscodeTileExtractionExecutorContainsTile(
                sourceCodec->WMISCP.uiTileX[tileColumn], sourceCodec->WMISCP.uiTileY[tileRow],
                macroblockLeft, macroblockRight, macroblockTop, macroblockBottom))
                for (packet = 0; packet < destinationPacketCount; ++packet, ++outputIndex)
                    destinationCodec->pIndexTable[outputIndex] =
                        sourceCodec->pIndexTable[(tileRow * sourceTileColumnCount + tileColumn) *
                            sourcePacketCount + packet + 1] -
                        sourceCodec->pIndexTable[(tileRow * sourceTileColumnCount + tileColumn) *
                            sourcePacketCount + packet];

    if (destinationCodec->WMISCP.cNumOfSliceMinus1H +
        destinationCodec->WMISCP.cNumOfSliceMinus1V == 0 &&
        destinationCodec->WMISCP.bfBitstreamFormat == SPATIAL) {
        destinationCodec->m_param.bIndexTable = FALSE;
        destinationCodec->cNumBitIO = 0;
        writeIndexTableNull(destinationCodec);
    }
    else writeIndexTable(destinationCodec);
    detachISWrite(destinationCodec, destinationCodec->pIOHeader);

    outputIndex = 0;
    for (tileRow = 0; tileRow < sourceCodec->WMISCP.cNumOfSliceMinus1H + 1; ++tileRow)
        for (tileColumn = 0; tileColumn < sourceCodec->WMISCP.cNumOfSliceMinus1V + 1;
            ++tileColumn)
            if (JxrTranscodeTileExtractionExecutorContainsTile(
                sourceCodec->WMISCP.uiTileX[tileColumn], sourceCodec->WMISCP.uiTileY[tileRow],
                macroblockLeft, macroblockRight, macroblockTop, macroblockBottom))
                for (packet = 0; packet < destinationPacketCount; ++packet) {
                    sourceCodec->WMISCP.pWStream->SetPos(sourceCodec->WMISCP.pWStream,
                        sourceCodec->pIndexTable[(tileRow * sourceTileColumnCount + tileColumn) *
                            sourcePacketCount + packet] + sourceCodec->cHeaderSize);
                    copyTo(sourceCodec->WMISCP.pWStream, destinationCodec->WMISCP.pWStream,
                        destinationCodec->pIndexTable[outputIndex++]);
                }
    free(destinationCodec->pIndexTable);
    destinationCodec->pIndexTable = NULL;
    return ICERR_OK;
}
