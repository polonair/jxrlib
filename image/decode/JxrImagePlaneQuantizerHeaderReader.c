#include "JxrImagePlaneQuantizerHeaderReader.h"

static Bool read_plane_quantizer(JxrDecoderBitSource* source, size_t channels, U8* indices,
    U8* mode)
{
    JxrDecoderQuantizerSyntax syntax;
    size_t channel;
    if (!JxrDecoderTileQuantizerSyntaxReadDc(source, channels, &syntax)) return FALSE;
    *mode = syntax.channelMode;
    indices[0] = syntax.indices[0];
    if (*mode == 1) indices[1] = syntax.indices[1];
    else if (*mode > 0) for (channel = 1; channel < channels; ++channel)
        indices[channel] = syntax.indices[channel];
    return TRUE;
}

Bool JxrImagePlaneQuantizerHeaderReaderRead(SimpleBitIO* input, size_t channelCount,
    SUBBAND subband, JxrImagePlaneQuantizerHeader* result)
{
    JxrDecoderBitSource source; U32 value; U8 mode;
    if (input == NULL || result == NULL || channelCount == 0 || channelCount >= MAX_CHANNELS)
        return FALSE;
    JxrDecoderBitSourceInitSimple(&source, input); result->quantizerMode = 0;
    result->hasDc = result->hasLp = result->hasHp = FALSE;
    result->dcMode = result->lpMode = result->hpMode = 0;
    if (!source.read(source.context, 1, &value)) return FALSE;
    if (value) { if (!read_plane_quantizer(&source, channelCount, result->dcIndices, &mode)) return FALSE; result->hasDc = TRUE; result->dcMode = mode; result->quantizerMode += mode << 3; }
    else ++result->quantizerMode;
    if (subband != SB_DC_ONLY) {
        if (!source.read(source.context, 1, &value)) return FALSE;
        if (!value) { result->quantizerMode += 0x200; if (!source.read(source.context, 1, &value)) return FALSE; if (value) { if (!read_plane_quantizer(&source, channelCount, result->lpIndices, &mode)) return FALSE; result->hasLp = TRUE; result->lpMode = mode; result->quantizerMode += mode << 5; } else result->quantizerMode += 2; }
        else result->quantizerMode += ((result->quantizerMode & 1) << 1) + ((result->quantizerMode & 0x18) << 2);
        if (subband != SB_NO_HIGHPASS) {
            if (!source.read(source.context, 1, &value)) return FALSE;
            if (!value) { result->quantizerMode += 0x400; if (!source.read(source.context, 1, &value)) return FALSE; if (value) { if (!read_plane_quantizer(&source, channelCount, result->hpIndices, &mode)) return FALSE; result->hasHp = TRUE; result->hpMode = mode; result->quantizerMode += mode << 7; } else result->quantizerMode += 4; }
            else result->quantizerMode += ((result->quantizerMode & 2) << 1) + ((result->quantizerMode & 0x60) << 2);
        }
    }
    if (subband == SB_DC_ONLY) result->quantizerMode |= 0x200;
    else if (subband == SB_NO_HIGHPASS) result->quantizerMode |= 0x400;
    return TRUE;
}
