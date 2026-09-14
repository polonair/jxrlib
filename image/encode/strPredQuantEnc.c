#include "strcodec.h"
#include "encode.h"
#include "JxrEncoderCoefficientPredictor.h"
#include "JxrEncoderCbpPredictor.h"

Void predMacroblockEnc(CWMImageStrCodec* pSC)
{
    JxrEncoderCoefficientPredictorApply(pSC);
}

Void predCBPEnc(CWMImageStrCodec* pSC, CCodingContext* pContext)
{
    JxrEncoderCbpPredictorApply(pSC, pContext);
}