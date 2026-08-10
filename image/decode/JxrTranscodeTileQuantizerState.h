#ifndef JXR_TRANSCODE_TILE_QUANTIZER_STATE_H
#define JXR_TRANSCODE_TILE_QUANTIZER_STATE_H

#include "strcodec.h"

#define JXR_TRANSCODE_MAX_QUANTIZERS 16

/* Tile-local quantizer metadata retained while a transcoder produces output tiles. */
typedef struct JxrTranscodeTileQuantizerState {
    U8 dcMode;
    U8 dcIndex[MAX_CHANNELS];
    Bool useDcForLowpass;
    U8 lowpassQuantizerCount;
    Bool useDcForLowpassAlpha;
    U8 lowpassQuantizerCountAlpha;
    U8 lowpassMode[JXR_TRANSCODE_MAX_QUANTIZERS];
    U8 lowpassIndex[JXR_TRANSCODE_MAX_QUANTIZERS][MAX_CHANNELS];
    Bool useLowpassForHighpass;
    U8 highpassQuantizerCount;
    Bool useLowpassForHighpassAlpha;
    U8 highpassQuantizerCountAlpha;
    U8 highpassMode[JXR_TRANSCODE_MAX_QUANTIZERS];
    U8 highpassIndex[JXR_TRANSCODE_MAX_QUANTIZERS][MAX_CHANNELS];
} JxrTranscodeTileQuantizerState;

Void JxrTranscodeTileQuantizerStateInit(JxrTranscodeTileQuantizerState* state);
Void JxrTranscodeTileQuantizerStateCapturePrimary(JxrTranscodeTileQuantizerState* state,
    const CWMITile* nativeTile, size_t channelCount, SUBBAND subband);
Void JxrTranscodeTileQuantizerStateCaptureAlpha(JxrTranscodeTileQuantizerState* state,
    const CWMITile* nativeTile, size_t alphaChannelIndex, SUBBAND subband);

#endif
