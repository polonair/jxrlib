#include "JxrEncoderPacketStreamInitializer.h"

#include "JxrEncoderPacketStreamCleanup.h"

Void JxrEncoderPacketStreamInitializationPlanInitialize(
    JxrEncoderPacketStreamInitializationPlan* plan,
    BITSTREAMFORMAT bitstreamFormat,
    U32 verticalSliceCountMinusOne,
    U32 horizontalSliceCountMinusOne,
    size_t packetStreamCount,
    size_t macroblockWidth,
    size_t macroblockHeight,
    size_t channelCount)
{
    plan->writesIndexTable = !(bitstreamFormat == SPATIAL &&
        verticalSliceCountMinusOne + horizontalSliceCountMinusOne == 0);
    plan->createsPacketStreams = packetStreamCount > 0;
    plan->usesTemporaryFiles = plan->createsPacketStreams &&
        macroblockWidth * macroblockHeight * channelCount >=
        JXR_ENCODER_MAX_MEMORY_SIZE_IN_WORDS;
}

static Int JxrEncoderPacketStreamInitializerAllocateStreams(
    CWMImageStrCodec* codec,
    const JxrEncoderPacketStreamInitializationPlan* plan)
{
    codec->ppWStream = (struct WMPStream**)malloc(
        codec->cNumBitIO * sizeof(struct WMPStream*));
    if (codec->ppWStream == NULL)
        return ICERR_ERROR;
    memset(codec->ppWStream, 0, codec->cNumBitIO * sizeof(struct WMPStream*));

    if (!plan->usesTemporaryFiles)
        return ICERR_OK;

#ifdef _WINDOWS_
    codec->ppTempFile = (TCHAR**)malloc(codec->cNumBitIO * sizeof(TCHAR*));
#else
    codec->ppTempFile = (char**)malloc(codec->cNumBitIO * sizeof(char*));
#endif
    if (codec->ppTempFile == NULL)
        return ICERR_ERROR;
#ifdef _WINDOWS_
    memset(codec->ppTempFile, 0, codec->cNumBitIO * sizeof(TCHAR*));
#else
    memset(codec->ppTempFile, 0, codec->cNumBitIO * sizeof(char*));
#endif
    return ICERR_OK;
}

static Int JxrEncoderPacketStreamInitializerCreateTemporaryStream(
    CWMImageStrCodec* codec,
    size_t streamIndex)
{
    char* fileName;
    struct WMPStream** streamSlot = &codec->ppWStream[streamIndex];

#if defined(_WINDOWS_) || defined(UNDER_CE)
    TCHAR temporaryPath[MAX_PATH];
    DWORD pathSize;
    DWORD sourceIndex;
    DWORD destinationIndex;
    Bool usesUnicode = sizeof(TCHAR) == 2;

    codec->ppTempFile[streamIndex] = (TCHAR*)malloc(MAX_PATH * sizeof(TCHAR));
    if (codec->ppTempFile[streamIndex] == NULL)
        return ICERR_ERROR;
    fileName = (char*)codec->ppTempFile[streamIndex];
    pathSize = GetTempPath(MAX_PATH, temporaryPath);
    if (pathSize == 0 || pathSize >= MAX_PATH)
        return ICERR_ERROR;
    if (!GetTempFileName(temporaryPath, TEXT("wdp"), 0,
        codec->ppTempFile[streamIndex]))
        return ICERR_ERROR;

    if (usesUnicode) {
        for (sourceIndex = destinationIndex = pathSize = 0;
            pathSize < MAX_PATH;
            ++pathSize, sourceIndex += 2) {
            if (codec->ppTempFile[streamIndex][pathSize] == '\0')
                break;
            if (fileName[sourceIndex] != '\0')
                fileName[destinationIndex++] = fileName[sourceIndex];
            if (fileName[sourceIndex + 1] != '\0')
                fileName[destinationIndex++] = fileName[sourceIndex + 1];
        }
        fileName[pathSize] = '\0';
    }
#else
    codec->ppTempFile[streamIndex] = (char*)malloc(FILENAME_MAX * sizeof(char));
    if (codec->ppTempFile[streamIndex] == NULL)
        return ICERR_ERROR;
    fileName = tmpnam(NULL);
    if (fileName == NULL)
        return ICERR_ERROR;
    strcpy(codec->ppTempFile[streamIndex], fileName);
#endif
    return CreateWS_File(streamSlot, fileName, "w+b") == ICERR_OK ?
        ICERR_OK : ICERR_ERROR;
}

static Int JxrEncoderPacketStreamInitializerCreateStreams(
    CWMImageStrCodec* codec,
    const JxrEncoderPacketStreamInitializationPlan* plan)
{
    size_t streamIndex;

    for (streamIndex = 0; streamIndex < codec->cNumBitIO; ++streamIndex) {
        struct WMPStream** streamSlot = &codec->ppWStream[streamIndex];
        Int result;

        if (plan->usesTemporaryFiles)
            result = JxrEncoderPacketStreamInitializerCreateTemporaryStream(codec,
                streamIndex);
        else
            result = CreateWS_List(streamSlot) == ICERR_OK ? ICERR_OK : ICERR_ERROR;
        if (result != ICERR_OK)
            return result;
        attachISWrite(codec->m_ppBitIO[streamIndex], codec->ppWStream[streamIndex]);
    }
    return ICERR_OK;
}

Int JxrEncoderPacketStreamInitializerInitialize(CWMImageStrCodec* codec)
{
    JxrEncoderPacketStreamInitializationPlan plan;
    Int result;

    JxrEncoderPacketStreamInitializationPlanInitialize(&plan,
        codec->WMISCP.bfBitstreamFormat, codec->WMISCP.cNumOfSliceMinus1V,
        codec->WMISCP.cNumOfSliceMinus1H, codec->cNumBitIO, codec->cmbWidth,
        codec->cmbHeight, codec->WMISCP.cChannel);
    codec->m_param.bIndexTable = plan.writesIndexTable;
    if (allocateBitIOInfo(codec) != ICERR_OK)
        return ICERR_ERROR;
    JxrEncoderPacketStreamInitializationPlanInitialize(&plan,
        codec->WMISCP.bfBitstreamFormat, codec->WMISCP.cNumOfSliceMinus1V,
        codec->WMISCP.cNumOfSliceMinus1H, codec->cNumBitIO, codec->cmbWidth,
        codec->cmbHeight, codec->WMISCP.cChannel);
    attachISWrite(codec->pIOHeader, codec->WMISCP.pWStream);
    if (!plan.createsPacketStreams)
        return ICERR_OK;

    result = JxrEncoderPacketStreamInitializerAllocateStreams(codec, &plan);
    if (result != ICERR_OK)
        return result;
    return JxrEncoderPacketStreamInitializerCreateStreams(codec, &plan);
}
