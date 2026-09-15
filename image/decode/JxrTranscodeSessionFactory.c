#include "JxrTranscodeSessionFactory.h"

Int JxrTranscodeSessionFactoryCreateCodec(struct WMPStream* stream,
    CWMImageStrCodec** codec)
{
    CWMImageStrCodec* allocatedCodec;

    if (stream == NULL || codec == NULL) return ICERR_ERROR;
    allocatedCodec = (CWMImageStrCodec*)malloc(sizeof(CWMImageStrCodec));
    if (allocatedCodec == NULL) return ICERR_ERROR;
    memset(allocatedCodec, 0, sizeof(CWMImageStrCodec));
    allocatedCodec->WMISCP.pWStream = stream;
    *codec = allocatedCodec;
    return ICERR_OK;
}
