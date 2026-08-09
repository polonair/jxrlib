#ifndef JXR_DECODER_SUBBAND_CONTEXT_H
#define JXR_DECODER_SUBBAND_CONTEXT_H

#include "strcodec.h"

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
    CAdaptiveModel* dcModel;
    CAdaptiveModel* lowpassModel;
    CAdaptiveModel* highpassModel;
    CAdaptiveHuffman** huffmanStates;
    CAdaptiveHuffman* cbpHuffman;
    CAdaptiveHuffman* cbpCountHuffman;
    CAdaptiveScan* lowpassScan;
    CAdaptiveScan* horizontalScan;
    CAdaptiveScan* verticalScan;
    Int* cbp;
    Int* differentialCbp;
} JxrDecoderSubbandContext;

Void JxrDecoderSubbandContextInit(JxrDecoderSubbandContext* state,
    CWMImageStrCodec* codec, CCodingContext* entropy);

#endif
