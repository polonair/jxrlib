#include "JxrEncoderMacroblockProcessingPipeline.h"

#include "JxrEncoderMacroblockProcessor.h"

Int JxrEncoderMacroblockProcessingPipelineProcessLoadedRow(CWMImageStrCodec* codec)
{
    if (JxrEncoderMacroblockProcessorProcess(codec) != ICERR_OK)
        return ICERR_ERROR;
    advanceMRPtr(codec);

    for (codec->cColumn = 1; codec->cColumn < codec->cmbWidth; ++codec->cColumn) {
        if (JxrEncoderMacroblockProcessorProcess(codec) != ICERR_OK)
            return ICERR_ERROR;
        advanceMRPtr(codec);
    }

    if (JxrEncoderMacroblockProcessorProcess(codec) != ICERR_OK)
        return ICERR_ERROR;
    if (codec->cRow)
        advanceOneMBRow(codec);

    ++codec->cRow;
    swapMRPtr(codec);
    return ICERR_OK;
}

Void JxrEncoderMacroblockProcessingPipelineProcessFinalRow(CWMImageStrCodec* codec)
{
    codec->cColumn = 0;
    initMRPtr(codec);

    JxrEncoderMacroblockProcessorProcess(codec);
    advanceMRPtr(codec);

    for (codec->cColumn = 1; codec->cColumn < codec->cmbWidth; ++codec->cColumn) {
        JxrEncoderMacroblockProcessorProcess(codec);
        advanceMRPtr(codec);
    }

    JxrEncoderMacroblockProcessorProcess(codec);
}
