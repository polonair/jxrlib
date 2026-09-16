#ifndef JXR_TRANSCODE_SESSION_H
#define JXR_TRANSCODE_SESSION_H

#include "JxrTranscodeTileQuantizerState.h"

/* Explicit ownership for all allocations made by WMPhotoTranscode. */
typedef struct JxrTranscodeSession {
    PixelI* macroblockBuffer;
    PixelI* primaryFrameBuffer;
    PixelI* alphaFrameBuffer;
    CWMIMBInfo* primaryFrameMacroblocks;
    CWMIMBInfo* alphaFrameMacroblocks;
    CWMImageStrCodec* decoderCodec;
    CWMImageStrCodec* encoderCodec;
    U8* decoderIoHeader;
    U8* encoderIoHeader;
    JxrTranscodeTileQuantizerState* tileQuantizers;
    Bool hasOrientation;
    Bool hasAlphaFrame;
    Bool decoderHasAlpha;
    /* These stages make release safe while construction is still in progress. */
    Bool decoderPrimaryResourcesInitialized;
    Bool encoderOutputInitialized;
    Bool usedFastTileExtraction;
} JxrTranscodeSession;

Void JxrTranscodeSessionRelease(JxrTranscodeSession* session);

#endif
