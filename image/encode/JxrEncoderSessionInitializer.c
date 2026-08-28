#include "JxrEncoderSessionInitializer.h"
#include "JxrEncoderInputRowProcessor.h"

Void JxrEncoderSessionInitializerInitialize(CWMImageStrCodec* codec,
    const CWMImageInfo* image, const CWMIStrCodecParam* parameters)
{
    codec->cbStruct = sizeof(*codec);
    codec->WMII = *image;
    codec->WMISCP = *parameters;
    if (codec->WMISCP.nExpBias == 0)
        codec->WMISCP.nExpBias = 4 + 128;
    codec->WMISCP.nExpBias += 128;
    codec->cRow = 0;
    codec->cColumn = 0;
    codec->cmbWidth = (codec->WMII.cWidth + 15) / 16;
    codec->cmbHeight = (codec->WMII.cHeight + 15) / 16;
#if defined(WMP_OPT_SSE2) || defined(WMP_OPT_CC_ENC) || defined(WMP_OPT_TRFM_ENC)
    codec->Load = JxrEncoderInputRowProcessorProcess;
#endif
    codec->m_pNextSC = NULL;
    codec->m_bSecondary = FALSE;
}
