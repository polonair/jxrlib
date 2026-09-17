#include "JxrTranscodeDecoderRuntimeInitializer.h"
#include "JxrDecoderInputInitializer.h"
#include "JxrDecoderResourceInitializer.h"
#include "JxrTranscodeMacroblockBufferLayout.h"
#include "JxrTranscodeBitIoHeaderLayout.h"

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
    JxrTranscodeBitIoHeaderLayout ioHeaderLayout;

    if (decoderCodec == NULL || state == NULL || state->macroblockBuffer == NULL)
        return ICERR_ERROR;
    if (!JxrTranscodeBitIoHeaderLayoutInitialize(&ioHeaderLayout, 0,
        sizeof(BitIOInfo))) return ICERR_ERROR;
    state->ioHeaderAllocation = (U8*)malloc(ioHeaderLayout.allocationBytes);
    if (state->ioHeaderAllocation == NULL) return ICERR_ERROR;
    if (!JxrTranscodeBitIoHeaderLayoutInitialize(&ioHeaderLayout,
        (UINTPTR_T)state->ioHeaderAllocation, sizeof(BitIOInfo)) ||
        !JxrTranscodeBitIoHeaderLayoutBindCompatibilityPointer(decoderCodec,
            state->ioHeaderAllocation, &ioHeaderLayout)) {
        free(state->ioHeaderAllocation);
        state->ioHeaderAllocation = NULL;
        return ICERR_ERROR;
    }
    memset(state->ioHeaderAllocation, 0, ioHeaderLayout.allocationBytes);
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
