#include "JxrTranscodeSession.h"

#include "decode.h"
#include "JxrDecoderResourceInitializer.h"
#include "JxrTranscodeSecondaryPlaneLink.h"
#include "../encode/encode.h"

EXTERN_C Void StrIOEncTerm(CWMImageStrCodec* codec);

Void JxrTranscodeSessionRelease(JxrTranscodeSession* session)
{
    if (session == NULL) return;
    free(session->macroblockBuffer);
    free(session->primaryFrameBuffer);
    free(session->primaryFrameMacroblocks);
    free(session->alphaFrameBuffer);
    free(session->alphaFrameMacroblocks);
    if (session->decoderCodec != NULL) {
        if (session->decoderPrimaryResourcesInitialized) {
            freePredInfo(session->decoderCodec);
            freeTileInfo(session->decoderCodec);
            JxrDecoderResourceInitializerReleaseIo(session->decoderCodec);
            FreeCodingContextDec(session->decoderCodec);
        }
        if (session->decoderHasAlpha)
            JxrTranscodeSecondaryPlaneLinkReleaseAttached(session->decoderCodec);
        free(session->decoderCodec);
    }
    free(session->decoderIoHeader);
    if (session->encoderCodec != NULL) {
        if (session->encoderOutputInitialized && !session->usedFastTileExtraction) {
            freePredInfo(session->encoderCodec);
            freeTileInfo(session->encoderCodec);
            StrIOEncTerm(session->encoderCodec);
            FreeCodingContextEnc(session->encoderCodec);
        }
        JxrTranscodeSecondaryPlaneLinkReleaseAttached(session->encoderCodec);
        free(session->encoderCodec);
    }
    free(session->tileQuantizers);
    free(session->encoderIoHeader);
    memset(session, 0, sizeof(*session));
}
