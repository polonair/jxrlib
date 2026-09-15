#include "JxrTranscodeEncoderInitializer.h"
#include "JxrTranscodeSessionFactory.h"

static Int JxrTranscodeEncoderInitializerAllocateIoHeader(
    CWMImageStrCodec* encoderCodec, U8** ioHeaderAllocation)
{
    size_t allocationSize = (PACKETLENGTH * 4 - 1) + PACKETLENGTH * 4 +
        sizeof(BitIOInfo);
    U8* allocation = (U8*)malloc(allocationSize);

    if (allocation == NULL) return ICERR_ERROR;
    memset(allocation, 0, allocationSize);
    encoderCodec->pIOHeader = (BitIOInfo*)((U8*)ALIGNUP(allocation,
        PACKETLENGTH * 4) + PACKETLENGTH * 2);
    *ioHeaderAllocation = allocation;
    return ICERR_OK;
}

static Void JxrTranscodeEncoderInitializerNormalizeIndexTable(
    CWMImageStrCodec* decoderCodec, CWMTranscodingParam* parameters)
{
    size_t index;
    size_t packetCount = decoderCodec->cNumBitIO *
        (decoderCodec->WMISCP.cNumOfSliceMinus1H + 1);

    for (index = 1; index < packetCount; index++) {
        if (decoderCodec->pIndexTable[index] == 0 && index + 1 != packetCount)
            decoderCodec->pIndexTable[index] = decoderCodec->pIndexTable[index + 1];
        if (decoderCodec->pIndexTable[index] != 0 &&
            decoderCodec->pIndexTable[index] < decoderCodec->pIndexTable[index - 1])
            parameters->bIgnoreOverlap = FALSE;
    }
}

Int JxrTranscodeEncoderInitializerInitialize(CWMImageStrCodec* decoderCodec,
    struct WMPStream* outputStream, CWMTranscodingParam* parameters,
    JxrTranscodeEncoderInitializationResult* result)
{
    CWMImageStrCodec* encoderCodec;
    size_t channel;

    if (decoderCodec == NULL || outputStream == NULL || parameters == NULL ||
        result == NULL) return ICERR_ERROR;
    result->encoderCodec = NULL;
    result->ioHeaderAllocation = NULL;
    if (JxrTranscodeSessionFactoryCreateCodec(outputStream, &encoderCodec) != ICERR_OK)
        return ICERR_ERROR;

    encoderCodec->WMII = decoderCodec->WMII;
    encoderCodec->WMISCP = decoderCodec->WMISCP;
    encoderCodec->m_param = decoderCodec->m_param;
    encoderCodec->WMISCP.bfBitstreamFormat = parameters->bfBitstreamFormat;
    encoderCodec->m_param.cfColorFormat = encoderCodec->WMISCP.cfColorFormat;
    channel = encoderCodec->WMISCP.cfColorFormat == Y_ONLY ? 1 :
        (encoderCodec->WMISCP.cfColorFormat == YUV_444 ? 3 :
            encoderCodec->WMISCP.cChannel);
    encoderCodec->m_param.cNumChannels = channel;
    encoderCodec->m_param.bAlphaChannel = parameters->uAlphaMode > 0;
    encoderCodec->m_param.bTranscode = TRUE;
    if (parameters->sbSubband >= SB_MAX) parameters->sbSubband = SB_ALL;
    if (parameters->sbSubband > encoderCodec->WMISCP.sbSubband)
        encoderCodec->WMISCP.sbSubband = parameters->sbSubband;
    encoderCodec->m_bSecondary = FALSE;

    if (JxrTranscodeEncoderInitializerAllocateIoHeader(encoderCodec,
        &result->ioHeaderAllocation) != ICERR_OK) {
        free(encoderCodec);
        return ICERR_ERROR;
    }
    for (channel = 0; channel < encoderCodec->m_param.cNumChannels; channel++)
        encoderCodec->pPlane[channel] = decoderCodec->p1MBbuffer[channel];

    JxrTranscodeEncoderInitializerNormalizeIndexTable(decoderCodec, parameters);
    result->encoderCodec = encoderCodec;
    return ICERR_OK;
}
