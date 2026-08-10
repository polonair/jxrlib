#include "JxrTranscodeTileQuantizerState.h"

#include <assert.h>
#include <string.h>

Void JxrTranscodeTileQuantizerStateInit(JxrTranscodeTileQuantizerState* state)
{
    memset(state, 0, sizeof(*state));
}

Void JxrTranscodeTileQuantizerStateCapturePrimary(JxrTranscodeTileQuantizerState* state,
    const CWMITile* nativeTile, size_t channelCount, SUBBAND subband)
{
    size_t channel;
    size_t quantizer;

    assert(state != NULL);
    assert(nativeTile != NULL);
    assert(channelCount <= MAX_CHANNELS);
    assert(nativeTile->cNumQPLP <= JXR_TRANSCODE_MAX_QUANTIZERS);
    assert(nativeTile->cNumQPHP <= JXR_TRANSCODE_MAX_QUANTIZERS);

    state->dcMode = nativeTile->cChModeDC;
    for (channel = 0; channel < channelCount; ++channel)
        state->dcIndex[channel] = nativeTile->pQuantizerDC[channel][0].iIndex;
    if (subband == SB_DC_ONLY) return;

    state->useDcForLowpass = nativeTile->bUseDC;
    state->lowpassQuantizerCount = nativeTile->cNumQPLP;
    if (state->useDcForLowpass == FALSE)
        for (quantizer = 0; quantizer < state->lowpassQuantizerCount; ++quantizer) {
            state->lowpassMode[quantizer] = nativeTile->cChModeLP[quantizer];
            for (channel = 0; channel < channelCount; ++channel)
                state->lowpassIndex[quantizer][channel] = nativeTile->pQuantizerLP[channel][quantizer].iIndex;
        }
    if (subband == SB_NO_HIGHPASS) return;

    if (state->useDcForLowpass == FALSE) {
        state->useLowpassForHighpass = nativeTile->bUseLP;
        state->highpassQuantizerCount = nativeTile->cNumQPHP;
        if (state->useLowpassForHighpass == FALSE)
            for (quantizer = 0; quantizer < state->highpassQuantizerCount; ++quantizer) {
                state->highpassMode[quantizer] = nativeTile->cChModeHP[quantizer];
                for (channel = 0; channel < channelCount; ++channel)
                    state->highpassIndex[quantizer][channel] = nativeTile->pQuantizerHP[channel][quantizer].iIndex;
            }
    }
}

Void JxrTranscodeTileQuantizerStateCaptureAlpha(JxrTranscodeTileQuantizerState* state,
    const CWMITile* nativeTile, size_t alphaChannelIndex, SUBBAND subband)
{
    size_t quantizer;

    assert(state != NULL);
    assert(nativeTile != NULL);
    assert(alphaChannelIndex < MAX_CHANNELS);
    assert(nativeTile->cNumQPLP <= JXR_TRANSCODE_MAX_QUANTIZERS);
    assert(nativeTile->cNumQPHP <= JXR_TRANSCODE_MAX_QUANTIZERS);

    state->dcIndex[alphaChannelIndex] = nativeTile->pQuantizerDC[0][0].iIndex;
    if (subband == SB_DC_ONLY) return;

    state->useDcForLowpassAlpha = nativeTile->bUseDC;
    state->lowpassQuantizerCountAlpha = nativeTile->cNumQPLP;
    if (state->useDcForLowpassAlpha == FALSE)
        for (quantizer = 0; quantizer < state->lowpassQuantizerCountAlpha; ++quantizer)
            state->lowpassIndex[quantizer][alphaChannelIndex] = nativeTile->pQuantizerLP[0][quantizer].iIndex;
    if (subband == SB_NO_HIGHPASS) return;

    if (state->useDcForLowpassAlpha == FALSE) {
        state->useLowpassForHighpassAlpha = nativeTile->bUseLP;
        state->highpassQuantizerCountAlpha = nativeTile->cNumQPHP;
        if (state->useLowpassForHighpassAlpha == FALSE)
            for (quantizer = 0; quantizer < state->highpassQuantizerCountAlpha; ++quantizer)
                state->highpassIndex[quantizer][alphaChannelIndex] = nativeTile->pQuantizerHP[0][quantizer].iIndex;
    }
}
