#include "JxrSubbandStreamRefill.h"

Void JxrSubbandStreamRefillLevel1(CWMImageStrCodec* codec, JxrEntropyBitReader* reader)
{
    readIS_L1(codec, reader->legacyStream);
}

Void JxrSubbandStreamRefillLevel2(CWMImageStrCodec* codec, JxrEntropyBitReader* reader)
{
    readIS_L2(codec, reader->legacyStream);
}
