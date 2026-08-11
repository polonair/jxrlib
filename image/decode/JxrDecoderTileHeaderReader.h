#ifndef JXR_DECODER_TILE_HEADER_READER_H
#define JXR_DECODER_TILE_HEADER_READER_H

#include "strcodec.h"

typedef struct JxrDecoderTileHeaderReaderConfig {
    CWMImageStrCodec* primaryCodec;
    CWMImageStrCodec* secondaryCodec;
    BitIOInfo* dcInput;
    BitIOInfo* lpInput;
    BitIOInfo* hpInput;
    U8 subbandCount;
} JxrDecoderTileHeaderReaderConfig;

typedef Void (*JxrDecoderTileHeaderReaderReadSubband)(Void* context,
    CWMImageStrCodec* codec, BitIOInfo* input);
typedef struct JxrDecoderTileHeaderReaderOperations {
    Void* context;
    JxrDecoderTileHeaderReaderReadSubband readDc;
    JxrDecoderTileHeaderReaderReadSubband readLp;
    JxrDecoderTileHeaderReaderReadSubband readHp;
} JxrDecoderTileHeaderReaderOperations;

/* Dispatches headers in legacy DC, LP, HP order for primary then alpha. */
Bool JxrDecoderTileHeaderReaderRead(const JxrDecoderTileHeaderReaderConfig* config,
    const JxrDecoderTileHeaderReaderOperations* operations);

#endif
