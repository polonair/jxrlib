#include "JxrDecoderSubbandContext.h"

Void JxrDecoderSubbandContextInit(JxrDecoderSubbandContext* state,
    CWMImageStrCodec* codec, CCodingContext* entropy)
{
    state->codec = codec;
    JxrMacroblockStateInit(&state->macroblockState, &codec->MBInfo);
    JxrCoefficientPlaneStateInit(&state->coefficientPlanes, codec->p1MBbuffer);
    JxrEntropyBitReaderInit(&state->dcReader, entropy->m_pIODC);
    JxrEntropyBitReaderInit(&state->lowpassReader, entropy->m_pIOLP);
    JxrEntropyBitReaderInit(&state->highpassReader, entropy->m_pIOAC);
    JxrEntropyBitReaderInit(&state->flexbitsReader, entropy->m_pIOFL);
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
