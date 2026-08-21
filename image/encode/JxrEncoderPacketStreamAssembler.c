#include "JxrEncoderPacketStreamAssembler.h"

#include "JxrEncoderIndexTableWriter.h"

Void JxrEncoderPacketStreamPlanInitialize(
    JxrEncoderPacketStreamPlan* plan,
    BITSTREAMFORMAT bitstreamFormat,
    Bool progressiveMode,
    U8 subbandPacketCount,
    U32 horizontalSliceCountMinusOne,
    U32 verticalSliceCountMinusOne)
{
    plan->usesSpatialLayout = bitstreamFormat == SPATIAL;
    plan->usesProgressiveFrequencyLayout = !plan->usesSpatialLayout && progressiveMode;
    plan->packetGroupCount = plan->usesProgressiveFrequencyLayout ?
        subbandPacketCount : 1;
    plan->horizontalTileCount = horizontalSliceCountMinusOne + 1;
    plan->verticalTileCount = verticalSliceCountMinusOne + 1;
    plan->subbandPacketCount = subbandPacketCount;
}

Int copyTo(struct WMPStream* source, struct WMPStream* destination, size_t byteCount)
{
    char buffer[PACKETLENGTH];

    if (byteCount <= JXR_ENCODER_MINIMUM_PACKET_LENGTH) {
        source->Read(source, buffer, byteCount);
        return ICERR_OK;
    }

    while (byteCount > PACKETLENGTH) {
        source->Read(source, buffer, PACKETLENGTH);
        destination->Write(destination, buffer, PACKETLENGTH);
        byteCount -= PACKETLENGTH;
    }
    source->Read(source, buffer, byteCount);
    destination->Write(destination, buffer, byteCount);
    return ICERR_OK;
}

static Void JxrEncoderPacketStreamAssemblerPrepareSources(CWMImageStrCodec* codec)
{
    size_t streamIndex;

    for (streamIndex = 0; streamIndex < codec->cNumBitIO; ++streamIndex)
        detachISWrite(codec, codec->m_ppBitIO[streamIndex]);
    for (streamIndex = 0; streamIndex < codec->cNumBitIO; ++streamIndex)
        codec->ppWStream[streamIndex]->SetPos(codec->ppWStream[streamIndex], 0);
}

static Void JxrEncoderPacketStreamAssemblerWriteSpatialTile(
    CWMImageStrCodec* codec,
    size_t verticalTileIndex,
    size_t* tableIndex)
{
    copyTo(codec->ppWStream[verticalTileIndex], codec->WMISCP.pWStream,
        codec->pIndexTable[(*tableIndex)++]);
}

static Void JxrEncoderPacketStreamAssemblerWriteFrequencyTile(
    CWMImageStrCodec* codec,
    size_t verticalTileIndex,
    size_t* tableIndex)
{
    size_t subbandIndex;

    for (subbandIndex = 0; subbandIndex < codec->cSB; ++subbandIndex)
        copyTo(codec->ppWStream[verticalTileIndex * codec->cSB + subbandIndex],
            codec->WMISCP.pWStream, codec->pIndexTable[(*tableIndex)++]);
}

static Void JxrEncoderPacketStreamAssemblerWriteProgressiveTile(
    CWMImageStrCodec* codec,
    size_t verticalTileIndex,
    size_t packetGroupIndex,
    size_t* tableIndex)
{
    copyTo(codec->ppWStream[verticalTileIndex * codec->cSB + packetGroupIndex],
        codec->WMISCP.pWStream, codec->pIndexTable[*tableIndex]);
    *tableIndex += codec->cSB;
}

Int JxrEncoderPacketStreamAssemblerAssemble(CWMImageStrCodec* codec)
{
    JxrEncoderPacketStreamPlan plan;
    size_t packetGroupIndex;

    if (codec->cNumBitIO == 0)
        return ICERR_OK;

    JxrEncoderPacketStreamPlanInitialize(&plan, codec->WMISCP.bfBitstreamFormat,
        codec->WMISCP.bProgressiveMode, codec->cSB,
        codec->WMISCP.cNumOfSliceMinus1H, codec->WMISCP.cNumOfSliceMinus1V);
    JxrEncoderPacketStreamAssemblerPrepareSources(codec);

    for (packetGroupIndex = 0;
        packetGroupIndex < plan.packetGroupCount;
        ++packetGroupIndex) {
        size_t horizontalTileIndex;
        size_t tableIndex = packetGroupIndex;

        for (horizontalTileIndex = 0;
            horizontalTileIndex < plan.horizontalTileCount;
            ++horizontalTileIndex) {
            size_t verticalTileIndex;

            for (verticalTileIndex = 0;
                verticalTileIndex < plan.verticalTileCount;
                ++verticalTileIndex) {
                if (plan.usesSpatialLayout)
                    JxrEncoderPacketStreamAssemblerWriteSpatialTile(codec,
                        verticalTileIndex, &tableIndex);
                else if (!plan.usesProgressiveFrequencyLayout)
                    JxrEncoderPacketStreamAssemblerWriteFrequencyTile(codec,
                        verticalTileIndex, &tableIndex);
                else
                    JxrEncoderPacketStreamAssemblerWriteProgressiveTile(codec,
                        verticalTileIndex, packetGroupIndex, &tableIndex);
            }
        }
    }
    return ICERR_OK;
}
