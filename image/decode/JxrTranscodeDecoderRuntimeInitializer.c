#include "JxrTranscodeDecoderRuntimeInitializer.h"
#include "JxrDecoderInputInitializer.h"
#include "JxrDecoderResourceInitializer.h"

Int JxrTranscodeDecoderRuntimeInitializerAllocateMacroblockBuffer(
    CWMImageStrCodec* decoderCodec, size_t coefficientUnit,
    JxrTranscodeDecoderRuntimeState* state)
{
    size_t channel;
    size_t channelOffset;

    if (decoderCodec == NULL || state == NULL || coefficientUnit == 0)
        return ICERR_ERROR;
    memset(state, 0, sizeof(*state));
    state->macroblockBuffer = (PixelI*)malloc(coefficientUnit * sizeof(PixelI));
    if (state->macroblockBuffer == NULL) return ICERR_ERROR;
    decoderCodec->p1MBbuffer[0] = state->macroblockBuffer;
    decoderCodec->p1MBbuffer[1] = decoderCodec->p1MBbuffer[0] + 256;
    channelOffset = decoderCodec->m_param.cfColorFormat == YUV_420 ? 64 :
        (decoderCodec->m_param.cfColorFormat == YUV_422 ? 128 : 256);
    for (channel = 2; channel < decoderCodec->m_param.cNumChannels; channel++)
        decoderCodec->p1MBbuffer[channel] = decoderCodec->p1MBbuffer[channel - 1] +
            channelOffset;
    return ICERR_OK;
}

Int JxrTranscodeDecoderRuntimeInitializerInitializePrimaryInput(
    CWMImageStrCodec* decoderCodec, JxrTranscodeDecoderRuntimeState* state)
{
    size_t allocationSize = (PACKETLENGTH * 4 - 1) + PACKETLENGTH * 4 +
        sizeof(BitIOInfo);

    if (decoderCodec == NULL || state == NULL || state->macroblockBuffer == NULL)
        return ICERR_ERROR;
    state->ioHeaderAllocation = (U8*)malloc(allocationSize);
    if (state->ioHeaderAllocation == NULL) return ICERR_ERROR;
    memset(state->ioHeaderAllocation, 0, allocationSize);
    decoderCodec->pIOHeader = (BitIOInfo*)((U8*)ALIGNUP(state->ioHeaderAllocation,
        PACKETLENGTH * 4) + PACKETLENGTH * 2);
    if (JxrDecoderInputInitializerInitialize(decoderCodec) != ICERR_OK ||
        JxrDecoderResourceInitializerInitialize(decoderCodec) != ICERR_OK)
        return ICERR_ERROR;
    return ICERR_OK;
}

Void JxrTranscodeDecoderRuntimeInitializerRelease(
    JxrTranscodeDecoderRuntimeState* state)
{
    if (state == NULL) return;
    free(state->macroblockBuffer);
    free(state->ioHeaderAllocation);
    memset(state, 0, sizeof(*state));
}
