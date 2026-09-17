#include "JxrTranscodeDecoderRuntimeInitializer.h"
#include "JxrDecoderInputInitializer.h"
#include "JxrDecoderResourceInitializer.h"
#include "JxrTranscodeMacroblockBufferLayout.h"

Int JxrTranscodeDecoderRuntimeInitializerAllocateMacroblockBuffer(
    CWMImageStrCodec* decoderCodec, size_t coefficientUnit,
    JxrTranscodeDecoderRuntimeState* state)
{
    JxrTranscodeMacroblockBufferLayout macroblockLayout;

    if (decoderCodec == NULL || state == NULL || coefficientUnit == 0)
        return ICERR_ERROR;
    if (!JxrTranscodeMacroblockBufferLayoutInitialize(
        decoderCodec->m_param.cfColorFormat, decoderCodec->m_param.cNumChannels,
        &macroblockLayout) || macroblockLayout.coefficientCount != coefficientUnit)
        return ICERR_ERROR;
    memset(state, 0, sizeof(*state));
    state->macroblockBuffer = (PixelI*)malloc(macroblockLayout.coefficientCount *
        sizeof(PixelI));
    if (state->macroblockBuffer == NULL) return ICERR_ERROR;
    if (!JxrTranscodeMacroblockBufferLayoutBindCompatibilityPointers(decoderCodec,
        state->macroblockBuffer, &macroblockLayout)) {
        free(state->macroblockBuffer);
        state->macroblockBuffer = NULL;
        return ICERR_ERROR;
    }
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
