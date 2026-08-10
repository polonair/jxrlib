#include "JxrDecoderSubbandContext.h"

Void JxrDecoderSubbandContextInit(JxrDecoderSubbandContext* state,
    CWMImageStrCodec* codec, CCodingContext* entropy)
{
    state->codec = codec;
    JxrEntropyBitReaderInit(&state->dcReader, entropy->m_pIODC);
    JxrEntropyBitReaderInit(&state->lowpassReader, entropy->m_pIOLP);
    JxrEntropyBitReaderInit(&state->highpassReader, entropy->m_pIOAC);
    JxrEntropyBitReaderInit(&state->flexbitsReader, entropy->m_pIOFL);
    JxrAdaptiveModelStateInit(&state->dcModelState, &entropy->m_aModelDC);
    JxrAdaptiveModelStateInit(&state->lowpassModelState, &entropy->m_aModelLP);
    JxrAdaptiveModelStateInit(&state->highpassModelState, &entropy->m_aModelAC);
    JxrHuffmanStateSetInit(&state->huffmanStateSet, entropy->m_pAHexpt);
    state->cbpHuffman = entropy->m_pAdaptHuffCBPCY;
    state->cbpCountHuffman = entropy->m_pAdaptHuffCBPCY1;
    state->highpassCbpModel = &entropy->m_aCBPModel;
    state->trimFlexBits = entropy->m_iTrimFlexBits;
    JxrLowpassCbpStateInit(&state->lowpassCbpState, &entropy->m_iCBPCountZero,
        &entropy->m_iCBPCountMax);
    state->lowpassScan = entropy->m_aScanLowpass;
    state->horizontalScan = entropy->m_aScanHoriz;
    state->verticalScan = entropy->m_aScanVert;
    JxrMacroblockCbpStateInit(&state->macroblockCbpState, codec->MBInfo.iCBP,
        codec->MBInfo.iDiffCBP);
}
