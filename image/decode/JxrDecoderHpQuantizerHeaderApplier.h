#ifndef JXR_DECODER_HP_QUANTIZER_HEADER_APPLIER_H
#define JXR_DECODER_HP_QUANTIZER_HEADER_APPLIER_H

#include "JxrDecoderTileQuantizerSyntaxReader.h"

/* Applies parsed HP quantizer syntax to the current decoder tile. */
Bool JxrDecoderHpQuantizerHeaderApplierApply(CWMImageStrCodec* codec,
    const JxrDecoderQuantizerSetSyntax* syntax);

#endif
