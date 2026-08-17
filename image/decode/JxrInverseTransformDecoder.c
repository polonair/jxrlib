#include "strTransform.h"
#include "strcodec.h"
#include "decode.h"
#include "JxrInverseTransformDecoder.h"
#include "JxrInverseTransformAlternateCodecSetup.h"
#include "JxrInverseTransformAlternateMacroblock.h"
#include "JxrInverseTransformCodecInvocation.h"
#include "JxrInverseTransformNormalCodecSetup.h"
#include "JxrInverseTransformNormalMacroblock.h"

Int JxrInverseTransformDecoderProcessNormalMacroblock(CWMImageStrCodec* codec)
{
    JxrInverseTransformNormalCodecSetup setup;
    JxrInverseTransformCodecInvocation invocation;
    JxrInverseTransformNormalMacroblock macroblock;

    JxrInverseTransformNormalCodecSetupInitialize(&setup, codec);
    JxrInverseTransformCodecInvocationInitialize(&invocation, codec);
    JxrInverseTransformCodecInvocationAdvancePostProcessRow(&invocation,
        &setup.geometry, setup.postProcessParameters.enabled);
    JxrInverseTransformNormalMacroblockInitialize(&macroblock, &setup, &invocation);
    JxrInverseTransformNormalMacroblockProcess(&macroblock);

    return ICERR_OK;
}

Int JxrInverseTransformDecoderProcessAlternateMacroblock(CWMImageStrCodec* codec)
{
    JxrInverseTransformAlternateCodecSetup setup;
    JxrInverseTransformCodecInvocation invocation;
    JxrInverseTransformAlternateMacroblock macroblock;

    JxrInverseTransformAlternateCodecSetupInitialize(&setup, codec);
    JxrInverseTransformCodecInvocationInitialize(&invocation, codec);
    JxrInverseTransformCodecInvocationAdvancePostProcessRow(&invocation,
        &setup.geometry, setup.postProcessParameters.enabled);
    JxrInverseTransformAlternateMacroblockInitialize(&macroblock, &setup, &invocation);
    JxrInverseTransformAlternateMacroblockProcess(&macroblock);

    return ICERR_OK;
}
