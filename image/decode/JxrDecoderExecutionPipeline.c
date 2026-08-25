#include "JxrDecoderExecutionPipeline.h"

#include "decode.h"
#include "JxrDecoderMacroblockProcessingPipeline.h"
#include "JxrDecoderOutputPipeline.h"
#include "JxrDecoderTransformPipeline.h"

Int JxrDecoderExecutionPipelineRun(CWMImageStrCodec* codec, size_t macroblockRowCount,
    Bool usesLegacyLoadCallback
#ifdef REENTRANT_MODE
    , size_t* decodedLines
#endif
    )
{
    Bool useCenterTransform = FALSE;
    const size_t chromaElementCount = codec->m_param.cfColorFormat == YUV_420 ? 8 * 8 :
        (codec->m_param.cfColorFormat == YUV_422 ? 8 * 16 : 16 * 16);
    size_t channelIndex;

#ifdef REENTRANT_MODE
    for (codec->cRow = codec->WMIBI.uiFirstMBRow;
        codec->cRow <= codec->WMIBI.uiLastMBRow; codec->cRow++)
    {
        if (codec->cRow == 0 || codec->cRow == macroblockRowCount)
            useCenterTransform = FALSE;
        else
            useCenterTransform = TRUE;
#else
    codec->cRow = 0;
    for (codec->cRow = 0; codec->cRow <= macroblockRowCount; codec->cRow++)
    {
#endif
        codec->cColumn = 0;
        initMRPtr(codec);
        memset(codec->p1MBbuffer[0], 0,
            sizeof(PixelI) * 16 * 16 * codec->cmbWidth);
        for (channelIndex = 1; channelIndex < codec->m_param.cNumChannels; channelIndex++) {
            memset(codec->p1MBbuffer[channelIndex], 0,
                sizeof(PixelI) * chromaElementCount * codec->cmbWidth);
        }
        if (codec->m_pNextSC != NULL) {
            memset(codec->m_pNextSC->p1MBbuffer[0], 0,
                sizeof(PixelI) * 16 * 16 * codec->m_pNextSC->cmbWidth);
        }

        if (JxrDecoderMacroblockProcessingPipelineProcess(codec) != ICERR_OK)
            return ICERR_ERROR;
        advanceMRPtr(codec);

        JxrDecoderTransformPipelineSetCenterMacroblock(codec, useCenterTransform);
        for (codec->cColumn = 1; codec->cColumn < codec->cmbWidth; ++codec->cColumn) {
            if (JxrDecoderMacroblockProcessingPipelineProcess(codec) != ICERR_OK)
                return ICERR_ERROR;
            advanceMRPtr(codec);
        }

        JxrDecoderTransformPipelineSetCenterMacroblock(codec, FALSE);
        if (JxrDecoderMacroblockProcessingPipelineProcess(codec) != ICERR_OK)
            return ICERR_ERROR;

        if (codec->cRow != 0) {
            if (codec->m_Dparam->cThumbnailScale < 2 &&
                (codec->m_Dparam->bDecodeFullFrame ||
                (codec->cRow * 16 > codec->m_Dparam->cROITopY &&
                codec->cRow * 16 <= codec->m_Dparam->cROIBottomY + 16))) {
                if (usesLegacyLoadCallback) {
                    if (codec->Load(codec) != ICERR_OK)
                        return ICERR_ERROR;
                }
                else if (JxrDecoderOutputPipelineWriteStandardRow(codec) != ICERR_OK)
                    return ICERR_ERROR;
            }

            if (codec->m_Dparam->cThumbnailScale >= 2)
                JxrDecoderOutputPipelineWriteThumbnailRow(codec);
        }

        advanceOneMBRow(codec);
        swapMRPtr(codec);
#ifdef REENTRANT_MODE
        *decodedLines = codec->WMIBI.cLinesDecoded;
#else
        useCenterTransform = codec->cRow != macroblockRowCount - 1;
#endif
    }

    return ICERR_OK;
}
