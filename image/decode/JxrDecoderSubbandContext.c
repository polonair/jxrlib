#include "JxrDecoderSubbandContext.h"

Void JxrDecoderSubbandContextInit(JxrDecoderSubbandContext* state,
    CWMImageStrCodec* codec, CCodingContext* entropy)
{
    state->codec = codec;
    JxrDecoderFormatStateInit(&state->formatState, codec);
    JxrMacroblockStateInit(&state->macroblockState, &codec->MBInfo);
    JxrCoefficientPlaneStateInit(&state->coefficientPlanes, codec->p1MBbuffer);
    JxrSharedBitReaderStateInit(&state->dcSharedReaderState, entropy->m_pIODC);
    JxrEntropyBitReaderInitShared(&state->dcReader, &state->dcSharedReaderState);
    if (entropy->m_pIOLP == entropy->m_pIODC)
        JxrEntropyBitReaderInitShared(&state->lowpassReader, &state->dcSharedReaderState);
    else {
        JxrSharedBitReaderStateInit(&state->lowpassSharedReaderState, entropy->m_pIOLP);
        JxrEntropyBitReaderInitShared(&state->lowpassReader, &state->lowpassSharedReaderState);
    }
    if (entropy->m_pIOAC == entropy->m_pIODC)
        JxrEntropyBitReaderInitShared(&state->highpassReader, &state->dcSharedReaderState);
    else if (entropy->m_pIOAC == entropy->m_pIOLP)
        JxrEntropyBitReaderInitShared(&state->highpassReader, state->lowpassReader.sharedState);
    else {
        JxrSharedBitReaderStateInit(&state->highpassSharedReaderState, entropy->m_pIOAC);
        JxrEntropyBitReaderInitShared(&state->highpassReader, &state->highpassSharedReaderState);
    }
    if (entropy->m_pIOFL == entropy->m_pIODC)
        JxrEntropyBitReaderInitShared(&state->flexbitsReader, &state->dcSharedReaderState);
    else if (entropy->m_pIOFL == entropy->m_pIOLP)
        JxrEntropyBitReaderInitShared(&state->flexbitsReader, state->lowpassReader.sharedState);
    else if (entropy->m_pIOFL == entropy->m_pIOAC)
        JxrEntropyBitReaderInitShared(&state->flexbitsReader, state->highpassReader.sharedState);
    else {
        JxrSharedBitReaderStateInit(&state->flexbitsSharedReaderState, entropy->m_pIOFL);
        JxrEntropyBitReaderInitShared(&state->flexbitsReader, &state->flexbitsSharedReaderState);
    }
    JxrAdaptiveModelStateInit(&state->dcModelState, &entropy->m_aModelDC);
    JxrAdaptiveModelStateInit(&state->lowpassModelState, &entropy->m_aModelLP);
    JxrAdaptiveModelStateInit(&state->highpassModelState, &entropy->m_aModelAC);
    JxrHuffmanStateSetInit(&state->huffmanStateSet, entropy->m_pAHexpt);
    JxrHighpassCbpStateInit(&state->highpassCbpState, entropy->m_pAdaptHuffCBPCY,
        entropy->m_pAdaptHuffCBPCY1, &entropy->m_aCBPModel);
    state->trimFlexBits = entropy->m_iTrimFlexBits;
    JxrLowpassCbpStateInit(&state->lowpassCbpState, &entropy->m_iCBPCountZero,
        &entropy->m_iCBPCountMax);
    JxrAdaptiveScanStateInit(&state->lowpassScanState, entropy->m_aScanLowpass);
    JxrAdaptiveScanStateInit(&state->horizontalScanState, entropy->m_aScanHoriz);
    JxrAdaptiveScanStateInit(&state->verticalScanState, entropy->m_aScanVert);
    JxrMacroblockCbpStateInit(&state->macroblockCbpState, codec->MBInfo.iCBP,
        codec->MBInfo.iDiffCBP);
}
