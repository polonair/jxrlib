#ifndef JXR_DECODER_DEQUANTIZER_H
#define JXR_DECODER_DEQUANTIZER_H

#include "strcodec.h"

/* Writes dequantized low-pass coefficients into one macroblock buffer. */
Void JxrDecoderDequantizerWrite4x4(PixelI* destination,
    const Int* coefficients, const Int* coefficientIndexes, Int quantizationParameter);
Void JxrDecoderDequantizerWrite4x2(PixelI* destination,
    const Int* coefficients, Int quantizationParameter);
Void JxrDecoderDequantizerWrite2x2(PixelI* destination,
    const Int* coefficients, Int quantizationParameter);

/* Dequantizes the DC and applicable LP coefficients of the current macroblock. */
Int JxrDecoderDequantizerDequantizeMacroblock(CWMImageStrCodec* codec);

#endif
