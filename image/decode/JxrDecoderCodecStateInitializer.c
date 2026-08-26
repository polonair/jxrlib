#include "JxrDecoderCodecStateInitializer.h"
#include "JxrDecoderOutputPipeline.h"
#include "JxrDecoderTransformPipeline.h"
#include <string.h>

Void JxrDecoderCodecStateInitializerInitialize(CWMImageStrCodec* codec,
    const CCoreParameters* parameters, const CWMImageStrCodec* templateCodec)
{
    memcpy(&codec->m_param, parameters, sizeof(CCoreParameters));

    codec->cbStruct = sizeof(*codec);
    codec->WMII = templateCodec->WMII;
    codec->WMISCP = templateCodec->WMISCP;
    codec->cRow = 0;
    codec->cColumn = 0;
    codec->cmbWidth = (codec->WMII.cWidth + 15) / 16;
    codec->cmbHeight = (codec->WMII.cHeight + 15) / 16;

#if defined(WMP_OPT_SSE2) || defined(WMP_OPT_CC_DEC) || defined(WMP_OPT_TRFM_DEC)
    codec->Load = JxrDecoderOutputPipelineWriteStandardRow;
#endif
    JxrDecoderTransformPipelineInitialize(codec,
        parameters->cSubVersion != CODEC_SUBVERSION);
    codec->m_pNextSC = NULL;
    codec->m_bSecondary = FALSE;
}
