#ifndef JXR_DECODER_LP_QUANTIZER_HEADER_APPLIER_H
#define JXR_DECODER_LP_QUANTIZER_HEADER_APPLIER_H

#include "JxrDecoderTileQuantizerSyntaxReader.h"

/* Applies parsed LP quantizer syntax to the current decoder tile. */
Bool JxrDecoderLpQuantizerHeaderApplierApply(CWMImageStrCodec* codec,
    const JxrDecoderQuantizerSetSyntax* syntax);

#endif
