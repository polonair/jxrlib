#ifndef JXR_ENCODER_MACROBLOCK_PROCESSING_PIPELINE_H
#define JXR_ENCODER_MACROBLOCK_PROCESSING_PIPELINE_H

#include "strcodec.h"

/* Process the macroblocks retained after one input row has been loaded. */
Int JxrEncoderMacroblockProcessingPipelineProcessLoadedRow(CWMImageStrCodec* codec);

/* Process the final buffered macroblock row before encoder shutdown. */
Void JxrEncoderMacroblockProcessingPipelineProcessFinalRow(CWMImageStrCodec* codec);

#endif
