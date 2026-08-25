#include "JxrDecoderMacroblockProcessingPipeline.h"

/* The macroblock syntax reader remains in strdec.c until its packet helpers move with it. */
Int processMacroblockDec(CWMImageStrCodec* codec);

Int JxrDecoderMacroblockProcessingPipelineProcess(CWMImageStrCodec* codec)
{
    return processMacroblockDec(codec);
}
