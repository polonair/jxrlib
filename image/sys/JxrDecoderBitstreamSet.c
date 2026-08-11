#include "JxrDecoderBitstreamSet.h"

static U32 JxrDecoderBitstreamSetGetSubbandCount(SUBBAND subband)
{
    if (subband == SB_DC_ONLY) return 1;
    if (subband == SB_NO_HIGHPASS) return 2;
    if (subband == SB_NO_FLEXBITS) return 3;
    return 4;
}

Bool JxrDecoderBitstreamSetInit(JxrDecoderBitstreamSet* bitstreams,
    Bool hasIndexTable, BITSTREAMFORMAT format, U32 verticalSlicesMinusOne,
    U32 horizontalSlicesMinusOne, SUBBAND subband)
{
    U32 subbandCount;

    if (bitstreams == NULL || verticalSlicesMinusOne >= MAX_TILES ||
        horizontalSlicesMinusOne >= MAX_TILES) return FALSE;
    if (!hasIndexTable && (format != SPATIAL || verticalSlicesMinusOne != 0 ||
        horizontalSlicesMinusOne != 0)) return FALSE;

    subbandCount = JxrDecoderBitstreamSetGetSubbandCount(subband);
    bitstreams->tileColumnCount = verticalSlicesMinusOne + 1;
    bitstreams->subbandCount = subbandCount;
    bitstreams->bitstreamsPerTile = format == SPATIAL ? 1 : subbandCount;
    bitstreams->usesHeaderStream = !hasIndexTable;
    bitstreams->bitstreamCount = hasIndexTable ?
        bitstreams->tileColumnCount * bitstreams->bitstreamsPerTile : 0;
    return bitstreams->bitstreamCount <= MAX_TILES * 4;
}

Bool JxrDecoderBitstreamSetBindTile(const JxrDecoderBitstreamSet* bitstreams,
    BitIOInfo* header, BitIOInfo** inputs, U32 tileColumn,
    JxrDecoderTileBitstreams* tileBitstreams)
{
    U32 base;

    if (bitstreams == NULL || tileBitstreams == NULL || tileColumn >= bitstreams->tileColumnCount)
        return FALSE;
    if (bitstreams->usesHeaderStream) {
        if (header == NULL) return FALSE;
        tileBitstreams->dc = header;
        tileBitstreams->lp = header;
        tileBitstreams->hp = header;
        tileBitstreams->flexbits = header;
        return TRUE;
    }
    if (inputs == NULL) return FALSE;
    base = tileColumn * bitstreams->bitstreamsPerTile;
    tileBitstreams->dc = inputs[base];
    tileBitstreams->lp = bitstreams->bitstreamsPerTile > 1 ? inputs[base + 1] : tileBitstreams->dc;
    tileBitstreams->hp = bitstreams->bitstreamsPerTile > 2 ? inputs[base + 2] : tileBitstreams->lp;
    tileBitstreams->flexbits = bitstreams->bitstreamsPerTile > 3 ? inputs[base + 3] : tileBitstreams->hp;
    return TRUE;
}
