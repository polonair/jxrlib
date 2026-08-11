#include "JxrDecoderTileHeaderReader.h"

static Void JxrDecoderTileHeaderReaderReadForCodecs(
    JxrDecoderTileHeaderReaderReadSubband readSubband, Void* context,
    CWMImageStrCodec* primaryCodec, CWMImageStrCodec* secondaryCodec, BitIOInfo* input)
{
    readSubband(context, primaryCodec, input);
    if (secondaryCodec != NULL) readSubband(context, secondaryCodec, input);
}

Bool JxrDecoderTileHeaderReaderRead(const JxrDecoderTileHeaderReaderConfig* config,
    const JxrDecoderTileHeaderReaderOperations* operations)
{
    if (config == NULL || operations == NULL || config->primaryCodec == NULL ||
        config->dcInput == NULL || operations->readDc == NULL)
        return FALSE;

    if (config->subbandCount > 1 &&
        (config->lpInput == NULL || operations->readLp == NULL))
        return FALSE;
    if (config->subbandCount > 2 &&
        (config->hpInput == NULL || operations->readHp == NULL))
        return FALSE;

    JxrDecoderTileHeaderReaderReadForCodecs(operations->readDc, operations->context,
        config->primaryCodec, config->secondaryCodec, config->dcInput);
    if (config->subbandCount > 1)
        JxrDecoderTileHeaderReaderReadForCodecs(operations->readLp, operations->context,
            config->primaryCodec, config->secondaryCodec, config->lpInput);
    if (config->subbandCount > 2)
        JxrDecoderTileHeaderReaderReadForCodecs(operations->readHp, operations->context,
            config->primaryCodec, config->secondaryCodec, config->hpInput);
    return TRUE;
}
