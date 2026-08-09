#include "JxrDecoderSubbandContext.h"

Void JxrDecoderSubbandContextInit(JxrDecoderSubbandContext* state,
    CWMImageStrCodec* codec, CCodingContext* entropy)
{
    state->codec = codec;
    state->entropy = entropy;
    state->dcInput = entropy->m_pIODC;
    state->lowpassInput = entropy->m_pIOLP;
    state->highpassInput = entropy->m_pIOAC;
    state->flexbitsInput = entropy->m_pIOFL;
    state->dcModel = &entropy->m_aModelDC;
    state->lowpassModel = &entropy->m_aModelLP;
    state->highpassModel = &entropy->m_aModelAC;
    state->huffmanStates = entropy->m_pAHexpt;
    state->cbpHuffman = entropy->m_pAdaptHuffCBPCY;
    state->cbpCountHuffman = entropy->m_pAdaptHuffCBPCY1;
    state->highpassCbpModel = &entropy->m_aCBPModel;
    state->lowpassCbpCountZero = &entropy->m_iCBPCountZero;
    state->lowpassCbpCountMax = &entropy->m_iCBPCountMax;
    state->lowpassScan = entropy->m_aScanLowpass;
    state->horizontalScan = entropy->m_aScanHoriz;
    state->verticalScan = entropy->m_aScanVert;
    state->cbp = codec->MBInfo.iCBP;
    state->differentialCbp = codec->MBInfo.iDiffCBP;
}
