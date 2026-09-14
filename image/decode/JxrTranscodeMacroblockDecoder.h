#ifndef JXR_TRANSCODE_MACROBLOCK_DECODER_H
#define JXR_TRANSCODE_MACROBLOCK_DECODER_H

#include "strcodec.h"

/* Decodes the primary macroblock and, when present, its secondary alpha plane. */
Int JxrTranscodeMacroblockDecoderDecode(CWMImageStrCodec* primaryCodec,
    Int macroblockColumn, Int macroblockRow);

#endif
