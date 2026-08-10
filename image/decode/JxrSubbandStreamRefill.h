#ifndef JXR_SUBBAND_STREAM_REFILL_H
#define JXR_SUBBAND_STREAM_REFILL_H

#include "JxrEntropyReader.h"

Bool JxrSubbandStreamRefillLevel1(CWMImageStrCodec* codec, JxrEntropyBitReader* reader);
Bool JxrSubbandStreamRefillLevel2(CWMImageStrCodec* codec, JxrEntropyBitReader* reader);

#endif
