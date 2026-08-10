#ifndef JXR_SUBBAND_STREAM_REFILL_H
#define JXR_SUBBAND_STREAM_REFILL_H

#include "JxrEntropyReader.h"

Void JxrSubbandStreamRefillLevel1(CWMImageStrCodec* codec, JxrEntropyBitReader* reader);
Void JxrSubbandStreamRefillLevel2(CWMImageStrCodec* codec, JxrEntropyBitReader* reader);

#endif
