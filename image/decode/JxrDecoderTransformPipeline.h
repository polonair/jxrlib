#ifndef JXR_DECODER_TRANSFORM_PIPELINE_H
#define JXR_DECODER_TRANSFORM_PIPELINE_H

#include "strcodec.h"

Void JxrDecoderTransformPipelineInitialize(CWMImageStrCodec* codec,
    Bool usesAlternateOperators);
Void JxrDecoderTransformPipelineSetCenterMacroblock(CWMImageStrCodec* codec,
    Bool isCenterMacroblock);
Int JxrDecoderTransformPipelineApply(CWMImageStrCodec* codec);

#endif
