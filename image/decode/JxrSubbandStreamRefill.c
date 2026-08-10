#include "JxrSubbandStreamRefill.h"

Void JxrSubbandStreamRefillLevel1(CWMImageStrCodec* codec, JxrEntropyBitReader* reader)
{
    JxrLegacyBitReaderAdapterRefillLevel1(codec, &reader->adapter);
}

Void JxrSubbandStreamRefillLevel2(CWMImageStrCodec* codec, JxrEntropyBitReader* reader)
{
    JxrLegacyBitReaderAdapterRefillLevel2(codec, &reader->adapter);
}
