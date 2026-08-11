#ifndef JXR_DECODER_BITSTREAM_SET_H
#define JXR_DECODER_BITSTREAM_SET_H

#include "strcodec.h"

typedef struct JxrDecoderBitstreamSet {
    U32 tileColumnCount;
    U32 subbandCount;
    U32 bitstreamsPerTile;
    U32 bitstreamCount;
    Bool isSpatial;
    Bool usesHeaderStream;
} JxrDecoderBitstreamSet;

typedef struct JxrDecoderTileBitstreams {
    BitIOInfo* dc;
    BitIOInfo* lp;
    BitIOInfo* hp;
    BitIOInfo* flexbits;
} JxrDecoderTileBitstreams;

Bool JxrDecoderBitstreamSetInit(JxrDecoderBitstreamSet* bitstreams,
    Bool hasIndexTable, BITSTREAMFORMAT format, U32 verticalSlicesMinusOne,
    U32 horizontalSlicesMinusOne, SUBBAND subband);
Bool JxrDecoderBitstreamSetBindTile(const JxrDecoderBitstreamSet* bitstreams,
    BitIOInfo* header, BitIOInfo** inputs, U32 tileColumn,
    JxrDecoderTileBitstreams* tileBitstreams);

#endif
