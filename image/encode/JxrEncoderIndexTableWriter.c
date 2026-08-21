#include "JxrEncoderIndexTableWriter.h"

Void JxrEncoderIndexTablePlanInitialize(
    JxrEncoderIndexTablePlan* plan,
    size_t bitStreamCount,
    U32 horizontalSliceCountMinusOne,
    BITSTREAMFORMAT bitstreamFormat,
    Bool progressiveMode,
    U8 subbandPacketCount)
{
    plan->packetGroupCount = bitstreamFormat == FREQUENCY && progressiveMode ?
        subbandPacketCount : 1;
    plan->entryCount = bitStreamCount * (horizontalSliceCountMinusOne + 1);
}

static Void JxrEncoderIndexTableWriterWriteVariableLengthWord(
    BitIOInfo* bitWriter,
    Int escape,
    size_t value)
{
    if (escape) {
        assert(escape <= 0xff && escape > 0xfc);
        putBit16(bitWriter, escape, 8);
    }
    else if (value < 0xfb00)
        putBit16(bitWriter, (U32)value, 16);
    else {
        size_t highValue = value >> 16;

        if ((highValue >> 16) == 0)
            putBit16(bitWriter, 0xfb, 8);
        else {
            highValue >>= 16;
            putBit16(bitWriter, 0xfc, 8);
            putBit16(bitWriter, (U32)(highValue >> 16) & 0xffff, 16);
            putBit16(bitWriter, (U32)highValue & 0xffff, 16);
        }
        putBit16(bitWriter, (U32)highValue & 0xffff, 16);
        putBit16(bitWriter, (U32)value & 0xffff, 16);
    }
}

Int JxrEncoderIndexTableWriterWriteNull(CWMImageStrCodec* codec)
{
    BitIOInfo* bitWriter;

    if (codec->cNumBitIO != 0)
        return ICERR_OK;

    bitWriter = codec->pIOHeader;
    fillToByte(bitWriter);
    JxrEncoderIndexTableWriterWriteVariableLengthWord(bitWriter, 0, 4);
    putBit16(bitWriter, 111, 8);
    putBit16(bitWriter, 255, 8);
    putBit16(bitWriter, 1, 16);
    return ICERR_OK;
}

static Void JxrEncoderIndexTableWriterConvertOffsetsToLengths(
    CWMImageStrCodec* codec,
    const JxrEncoderIndexTablePlan* plan,
    size_t cumulativeSizes[4])
{
    size_t rowCount = codec->WMISCP.cNumOfSliceMinus1H + 1;

    while (rowCount > 0 && !codec->bTileExtraction) {
        size_t packetIndex = 0;
        size_t rowIndex = --rowCount;

        while (packetIndex < codec->cNumBitIO) {
            size_t groupIndex;

            for (groupIndex = 0;
                groupIndex < plan->packetGroupCount;
                ++groupIndex, ++packetIndex) {
                size_t tableIndex = codec->cNumBitIO * rowIndex + packetIndex;

                if (rowIndex > 0)
                    codec->pIndexTable[tableIndex] -= codec->pIndexTable[
                        codec->cNumBitIO * (rowIndex - 1) + packetIndex];
                cumulativeSizes[groupIndex] += codec->pIndexTable[tableIndex];
            }
        }
    }
}

static Void JxrEncoderIndexTableWriterMakeCumulativeOffsets(size_t sizes[4])
{
    sizes[3] = sizes[2] + sizes[1] + sizes[0];
    sizes[2] = sizes[1] + sizes[0];
    sizes[1] = sizes[0];
    sizes[0] = 0;
}

Int JxrEncoderIndexTableWriterWrite(CWMImageStrCodec* codec)
{
    JxrEncoderIndexTablePlan plan;
    BitIOInfo* bitWriter;
    size_t cumulativeSizes[4] = { 0 };
    size_t entryIndex;

    if (codec->cNumBitIO == 0)
        return ICERR_OK;

    JxrEncoderIndexTablePlanInitialize(&plan, codec->cNumBitIO,
        codec->WMISCP.cNumOfSliceMinus1H, codec->WMISCP.bfBitstreamFormat,
        codec->WMISCP.bProgressiveMode, codec->cSB);
    bitWriter = codec->pIOHeader;
    putBit16(bitWriter, 1, 16);
    JxrEncoderIndexTableWriterConvertOffsetsToLengths(codec, &plan, cumulativeSizes);
    JxrEncoderIndexTableWriterMakeCumulativeOffsets(cumulativeSizes);

    entryIndex = 0;
    while (entryIndex < plan.entryCount) {
        size_t groupIndex;

        for (groupIndex = 0;
            groupIndex < plan.packetGroupCount;
            ++groupIndex, ++entryIndex) {
            size_t packetLength = codec->pIndexTable[entryIndex];
            Int escape = packetLength <= JXR_ENCODER_MINIMUM_PACKET_LENGTH ? 0xff : 0;

            writeIS_L1(codec, bitWriter);
            JxrEncoderIndexTableWriterWriteVariableLengthWord(bitWriter, escape,
                cumulativeSizes[groupIndex]);
            cumulativeSizes[groupIndex] += escape ? 0 : packetLength;
        }
    }

    writeIS_L1(codec, bitWriter);
    JxrEncoderIndexTableWriterWriteVariableLengthWord(bitWriter, 0xff, 0);
    fillToByte(bitWriter);
    return ICERR_OK;
}
