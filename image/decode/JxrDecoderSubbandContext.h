#ifndef JXR_DECODER_SUBBAND_CONTEXT_H
#define JXR_DECODER_SUBBAND_CONTEXT_H

#include "strcodec.h"
#include "JxrEntropyReader.h"

/*
 * Explicit view of the legacy state consumed by one macroblock subband
 * decoder.  It owns no memory; every field aliases the reference decoder.
 */
typedef struct JxrDecoderSubbandContext {
    CWMImageStrCodec* codec;
    CCodingContext* entropy;
    BitIOInfo* dcInput;
    BitIOInfo* lowpassInput;
    BitIOInfo* highpassInput;
    BitIOInfo* flexbitsInput;
    JxrEntropyBitReader dcReader;
    JxrEntropyBitReader lowpassReader;
    JxrEntropyBitReader highpassReader;
    JxrEntropyBitReader flexbitsReader;
    CAdaptiveModel* dcModel;
    CAdaptiveModel* lowpassModel;
    CAdaptiveModel* highpassModel;
    CAdaptiveHuffman** huffmanStates;
    CAdaptiveHuffman* cbpHuffman;
    CAdaptiveHuffman* cbpCountHuffman;
    CCBPModel* highpassCbpModel;
    Int trimFlexBits;
    Int* lowpassCbpCountZero;
    Int* lowpassCbpCountMax;
    CAdaptiveScan* lowpassScan;
    CAdaptiveScan* horizontalScan;
    CAdaptiveScan* verticalScan;
    Int* cbp;
    Int* differentialCbp;
} JxrDecoderSubbandContext;

Void JxrDecoderSubbandContextInit(JxrDecoderSubbandContext* state,
    CWMImageStrCodec* codec, CCodingContext* entropy);

#endif
