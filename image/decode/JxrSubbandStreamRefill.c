#include "JxrSubbandStreamRefill.h"

Bool JxrSubbandStreamRefillLevel1(CWMImageStrCodec* codec, JxrEntropyBitReader* reader)
{
    Bool succeeded = JxrSharedBitReaderStateRefillLevel1(codec, reader->sharedState);
    if (!succeeded) reader->hasError = TRUE;
    return succeeded;
}

Bool JxrSubbandStreamRefillLevel2(CWMImageStrCodec* codec, JxrEntropyBitReader* reader)
{
    Bool succeeded = JxrSharedBitReaderStateRefillLevel2(codec, reader->sharedState);
    if (!succeeded) reader->hasError = TRUE;
    return succeeded;
}
