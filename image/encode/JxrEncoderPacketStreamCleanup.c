#include "JxrEncoderPacketStreamCleanup.h"

Void JxrEncoderPacketStreamCleanupPlanInitialize(
    JxrEncoderPacketStreamCleanupPlan* plan,
    size_t packetStreamCount,
    size_t macroblockWidth,
    size_t macroblockHeight,
    size_t channelCount)
{
    plan->releasesPacketResources = packetStreamCount > 0;
    plan->releasesTemporaryFiles = plan->releasesPacketResources &&
        macroblockWidth * macroblockHeight * channelCount >=
        JXR_ENCODER_MAX_MEMORY_SIZE_IN_WORDS;
    plan->closesPacketStreams = plan->releasesPacketResources &&
        !plan->releasesTemporaryFiles;
}

static Int JxrEncoderPacketStreamCleanupReleaseTemporaryFiles(CWMImageStrCodec* codec)
{
    size_t streamIndex;

    for (streamIndex = 0; streamIndex < codec->cNumBitIO; ++streamIndex) {
        struct WMPStream* stream = codec->ppWStream == NULL ? NULL :
            codec->ppWStream[streamIndex];

        if (stream != NULL) {
            if (stream->state.file.pFile != NULL) {
                fclose(stream->state.file.pFile);
#ifdef _WINDOWS_
                if (DeleteFileA((LPCSTR)codec->ppTempFile[streamIndex]) == 0)
                    return ICERR_ERROR;
#else
                if (remove(codec->ppTempFile[streamIndex]) == -1)
                    return ICERR_ERROR;
#endif
            }
            free(stream);
        }
        if (codec->ppTempFile != NULL && codec->ppTempFile[streamIndex] != NULL)
            free(codec->ppTempFile[streamIndex]);
    }

    if (codec->ppTempFile != NULL)
        free(codec->ppTempFile);
    return ICERR_OK;
}

static Void JxrEncoderPacketStreamCleanupCloseStreams(CWMImageStrCodec* codec)
{
    size_t streamIndex;

    for (streamIndex = 0; streamIndex < codec->cNumBitIO; ++streamIndex) {
        struct WMPStream** streamSlot = codec->ppWStream == NULL ? NULL :
            &codec->ppWStream[streamIndex];

        if (streamSlot != NULL && *streamSlot != NULL)
            (*streamSlot)->Close(streamSlot);
    }
}

Int JxrEncoderPacketStreamCleanupRelease(CWMImageStrCodec* codec)
{
    JxrEncoderPacketStreamCleanupPlan plan;
    Int result;

    JxrEncoderPacketStreamCleanupPlanInitialize(&plan, codec->cNumBitIO,
        codec->cmbWidth, codec->cmbHeight, codec->WMISCP.cChannel);
    if (!plan.releasesPacketResources)
        return ICERR_OK;

    if (plan.releasesTemporaryFiles) {
        result = JxrEncoderPacketStreamCleanupReleaseTemporaryFiles(codec);
        if (result != ICERR_OK)
            return result;
    }
    else if (plan.closesPacketStreams)
        JxrEncoderPacketStreamCleanupCloseStreams(codec);

    free(codec->ppWStream);
    free(codec->m_ppBitIO);
    free(codec->pIndexTable);
    return ICERR_OK;
}
