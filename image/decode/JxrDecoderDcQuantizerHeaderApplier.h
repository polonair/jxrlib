#ifndef JXR_DECODER_DC_QUANTIZER_HEADER_APPLIER_H
#define JXR_DECODER_DC_QUANTIZER_HEADER_APPLIER_H

#include "JxrDecoderTileQuantizerSyntaxReader.h"

/*
 * Applies parsed DC quantizer syntax to the current decoder tile.  The native
 * codec remains the owner of the quantizer storage; this operation only makes
 * the allocation and state transition explicit.
 */
Bool JxrDecoderDcQuantizerHeaderApplierApply(CWMImageStrCodec* codec,
    const JxrDecoderQuantizerSyntax* syntax);

#endif
