#include "JxrDecoderTransformPipeline.h"

#include "JxrInverseTransformDecoder.h"

Void JxrDecoderTransformPipelineInitialize(CWMImageStrCodec* codec,
    Bool usesAlternateOperators)
{
    codec->m_bDecoderUseAlternateTransform = usesAlternateOperators;
    codec->m_bDecoderUseCenterTransform = FALSE;
    codec->TransformCenter = NULL;
}

Void JxrDecoderTransformPipelineSetCenterMacroblock(CWMImageStrCodec* codec,
    Bool isCenterMacroblock)
{
    codec->m_bDecoderUseCenterTransform = isCenterMacroblock;
}

Int JxrDecoderTransformPipelineApply(CWMImageStrCodec* codec)
{
    if (codec->m_bDecoderUseCenterTransform && codec->TransformCenter != NULL)
        return codec->TransformCenter(codec);

    if (codec->m_bDecoderUseAlternateTransform)
        return JxrInverseTransformDecoderProcessAlternateMacroblock(codec);

    return JxrInverseTransformDecoderProcessNormalMacroblock(codec);
}
