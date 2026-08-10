#include "JxrSubbandStreamRefill.h"

Void JxrSubbandStreamRefillLevel1(CWMImageStrCodec* codec, JxrEntropyBitReader* reader)
{
    JxrSharedBitReaderStateRefillLevel1(codec, reader->sharedState);
}

Void JxrSubbandStreamRefillLevel2(CWMImageStrCodec* codec, JxrEntropyBitReader* reader)
{
    JxrSharedBitReaderStateRefillLevel2(codec, reader->sharedState);
}
