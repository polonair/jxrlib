#ifndef JXR_TRANSCODE_MACROBLOCK_DECODER_H
#define JXR_TRANSCODE_MACROBLOCK_DECODER_H

#include "JxrTranscodePlanePair.h"

/* Decodes the primary macroblock and, when present, its secondary alpha plane. */
Int JxrTranscodeMacroblockDecoderDecode(const JxrTranscodePlanePair* planes,
    Int macroblockColumn, Int macroblockRow);

#endif
