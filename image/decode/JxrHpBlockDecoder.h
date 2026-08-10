#ifndef JXR_HP_BLOCK_DECODER_H
#define JXR_HP_BLOCK_DECODER_H

#include "JxrAdaptiveScan.h"
#include "JxrCoefficientBuffer.h"
#include "JxrEntropyReader.h"

typedef struct JxrHpBlockDecodingContext {
    CAdaptiveHuffman** huffmanStates;
    JxrEntropyBitReader* highpassReader;
    JxrEntropyBitReader* flexbitsReader;
    JxrCoefficientBuffer* coefficientBuffer;
    CAdaptiveScan* scan;
    const Int* coefficientOrder;
    Bool isChroma;
    Bool hasCoefficients;
    Bool skipFlexbits;
    Int modelBits;
    Int trimFlexBits;
    Int quantizationParameter;
} JxrHpBlockDecodingContext;

Int JxrHpBlockDecoderDecode(JxrHpBlockDecodingContext* context);

#endif
