#ifndef JXR_HP_BLOCK_DECODER_H
#define JXR_HP_BLOCK_DECODER_H

#include "JxrAdaptiveScanState.h"
#include "JxrCoefficientBuffer.h"
#include "JxrEntropyReader.h"
#include "JxrHuffmanStateSet.h"

typedef struct JxrHpBlockDecodingContext {
    JxrHuffmanStateSet* huffmanStateSet;
    JxrEntropyBitReader* highpassReader;
    JxrEntropyBitReader* flexbitsReader;
    JxrCoefficientBuffer* coefficientBuffer;
    JxrAdaptiveScanState* scanState;
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
