#include "JxrEncoderMacroblockProcessor.h"

#include "JXRTrace.h"
#include "encode.h"
#include "JxrForwardTransformEncoder.h"

Void JxrEncoderMacroblockProcessStateInitialize(
    JxrEncoderMacroblockProcessState* state,
    const CWMImageStrCodec* primaryCodec)
{
    state->encodesPreviousMacroblock = primaryCodec->cColumn != 0 &&
        primaryCodec->cRow != 0;
    state->processesSecondaryCodec = primaryCodec->m_pNextSC != NULL;
    state->previousMacroblockX = (Int)primaryCodec->cColumn - 1;
    state->previousMacroblockY = (Int)primaryCodec->cRow - 1;
}

static Int JxrEncoderMacroblockProcessorProcessCodec(
    CWMImageStrCodec* codec,
    const JxrEncoderMacroblockProcessState* state,
    CWMImageStrCodec* secondaryCodecToSynchronize)
{
    JxrForwardTransformEncoderProcessMacroblock(codec);
    if (codec->cColumn < codec->cmbWidth && codec->cRow < codec->cmbHeight) {
        JXRTraceDumpStage("encoder", "transform_coefficients", codec,
            (Int)codec->cColumn, (Int)codec->cRow, JXRTraceCoefficients);
    }

    if (!state->encodesPreviousMacroblock)
        return ICERR_OK;

    getTilePos(codec, state->previousMacroblockX, state->previousMacroblockY);
    if (secondaryCodecToSynchronize != NULL) {
        secondaryCodecToSynchronize->cTileRow = codec->cTileRow;
        secondaryCodecToSynchronize->cTileColumn = codec->cTileColumn;
    }
    return encodeMB(codec, state->previousMacroblockX, state->previousMacroblockY);
}

Int JxrEncoderMacroblockProcessorProcess(CWMImageStrCodec* primaryCodec)
{
    JxrEncoderMacroblockProcessState state;
    CWMImageStrCodec* secondaryCodec;
    Int result;

    JxrEncoderMacroblockProcessStateInitialize(&state, primaryCodec);
    secondaryCodec = state.processesSecondaryCodec ? primaryCodec->m_pNextSC : NULL;

    result = JxrEncoderMacroblockProcessorProcessCodec(primaryCodec, &state, secondaryCodec);
    if (result != ICERR_OK)
        return result;

    if (secondaryCodec == NULL)
        return ICERR_OK;

    secondaryCodec->cRow = primaryCodec->cRow;
    secondaryCodec->cColumn = primaryCodec->cColumn;
    return JxrEncoderMacroblockProcessorProcessCodec(secondaryCodec, &state, NULL);
}
